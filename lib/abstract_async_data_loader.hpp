#pragma once

#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <utility>
#include <memory>

#include "boost/tokenizer.hpp"

#include "base/magic.hpp"
#include "io/line_input_format.hpp"
#include "lib/labeled_sample.hpp"
#include "lib/LUrlParser/LUrlParser.hpp"
#include "lib/blockingconcurrentqueue.h"
#include "lib/parser.hpp"

namespace csci5570 {
namespace lib {

class AsyncReadBuffer {
public:
  using BatchT = std::vector<std::string>;
  ~AsyncReadBuffer() {
    thread_.join();
    // master_thread_.join();
  }

  /*
   * Initializes the input format and reader threads
   *
   * Consider the producer-consumer paradigm
   *
   * @param url         the file url (e.g. in hdfs)
   * @param task_id     identifier to this running task
   * @param num_threads the number of worker threads we are using
   * @param batch_size  the size of each batch
   * @param batch_num   the max number of records in the buffer
   */
  void init(const std::string &url, int task_id, int num_threads, std::string master_host, int master_port ,std::string worker_host,
            int batch_size) {
    
    if (init_) return ; // return if already init
    batch_size_ = batch_size;
    // 1. initialize URL      

    // parse url into name hdfs_namenode and port
    LUrlParser::clParseURL URL = LUrlParser::clParseURL::ParseURL( url );

    if ( ! URL.IsValid() ){ return ; } 
    std::string hdfs_namenode = URL.m_Host.empty() ? "localhost" : URL.m_Host ;
    int hdfs_namenode_port = URL.m_Port.empty() ? 9000 : stoi(URL.m_Port) ;
    std::string path = "/" + URL.m_Path; 
    
    DLOG(INFO) << "Host: " << hdfs_namenode ;
    DLOG(INFO) << "Port: " << hdfs_namenode_port ;
    DLOG(INFO) << "Path: " << path ;
    
    // 1. Spawn the HDFS block assigner thread on the master
        
    zmq_context = std::shared_ptr<zmq::context_t> (new zmq::context_t(1));
    int proc_id = getpid();
    
    coordinator = std::shared_ptr<Coordinator> (new Coordinator(proc_id, worker_host, zmq_context.get(), master_host, master_port));
    coordinator->serve();
    DLOG(INFO) << "Coordinator begins serving";
    
    // 2. spawn spreads to asynchronously load data
    DLOG(INFO) << "Spawning Producer Thread(s)";        
    
    thread_ = std::thread( [this, proc_id, path, num_threads,task_id, worker_host, hdfs_namenode, hdfs_namenode_port, master_port, master_host] () {
      
      DLOG(INFO) << "thread id:" << std::this_thread::get_id() << " started\n";
      
      infmt_ = std::shared_ptr<LineInputFormat> (new LineInputFormat(path, num_threads, task_id, coordinator.get(),
                                  worker_host, hdfs_namenode, hdfs_namenode_port));
      DLOG(INFO) << "Line input is well prepared";
       
      // put the line into the batch.
      bool success = false;
      boost::string_ref record;
      try
      {
        while (true) {
          success = infmt_->next(record); // is it thread safe?
          if (success) {
            std::string record_(record);
            boost::char_separator<char> sep{"\n"};
            boost::tokenizer<boost::char_separator<char> > tok{record_, sep};
            for (const auto t : tok)
            {
              buffer_.enqueue(std::move(t));
            }
            DLOG(INFO) << "Buffer size : " << buffer_.size_approx();
              
          } else {
              eof_ = true;
              DLOG(INFO) << "EOF Reached " ;
              break;
          }

        }

      } catch (const std::exception& e) {
         LOG(FATAL) << e.what();
         
      }
      
      // Remember to notify master that the worker wants to exit
      BinStream finish_signal;
      finish_signal << worker_host << task_id;
      coordinator->notify_master(finish_signal, 300);
      
      DLOG(INFO) << "thread id:" << std::this_thread::get_id() << " ended\n";
    });
        
    init_ = true;
  }

  bool get_batch(BatchT *batch) {
    // store batch_size_ records in <batch> and return true if success
    // i.e. Consumer
    for (int i = 0 ; i < batch_size_ ; i++) 
    {

    if (eof_ && buffer_.size_approx() == 0)
    {
        LOG(INFO) << "EOF " << eof_ << " buffer_.size_approx()" << buffer_.size_approx();
        return false;
    }
    std::string line;
    buffer_.wait_dequeue(line);
    batch->push_back(line);
    }
    return true;
  }

  int ask() {
    // return the number of batches buffered
    return buffer_.size_approx();
  }

  inline bool end_of_file() const {
    // return true if the end of the file is reached
    return eof_;
  }

protected:
  virtual void main() {
    // the workloads of the thread reading samples asynchronously
  }
    
  std::shared_ptr<zmq::context_t> zmq_context;
  std::shared_ptr<Coordinator> coordinator;

  // input
  std::shared_ptr<LineInputFormat> infmt_;
  std::atomic<bool> eof_{false};

  // buffer
  moodycamel::BlockingConcurrentQueue<std::string> buffer_;
  
  int batch_size_;      // the size of each batch
  // int batch_num_;       // max buffered batch number
  // int end_ = 0;         // writer appends to the end_
  // int start_ = 0;       // reader reads from the start_

  // thread
  std::thread thread_;
  // std::thread master_thread_;
  // std::mutex mutex_;
  // std::condition_variable load_cv_;
  // std::condition_variable get_cv_;
  
  bool init_ = false;
private: 
  
  
};

template <typename Sample, typename Parse> 
class AbstractAsyncDataLoader {
public:
  AbstractAsyncDataLoader(std::string url, int n_features, Parse parse, int batch_size,
                          AsyncReadBuffer *buffer, int second_id, int num_threads, std::string hdfs_master_host, int hdfs_master_port ,std::string worker_host)
      : batch_size_(batch_size), n_features_(n_features), buffer_(buffer), parse_(parse){
            
        // parse data in buffer
        buffer_->init(url, second_id, num_threads, hdfs_master_host, hdfs_master_port, worker_host, batch_size_);        
      }

  virtual const std::vector<Sample> &get_data() {
    std::vector<Sample> output; 
    AsyncReadBuffer::BatchT batch ;   
    if ( buffer_->get_batch(&batch) ){
      for(auto b : batch) { 
        Sample sample = parse_(b, n_features_);
        output.push_back(sample);
      }
    }
    return output;
    
  }
  // std::vector<Key> get_keys() {
  //   // return the keys of features of the current batch
  //   return std::vector<Key> (index_set_.begin(), index_set_.end());
  // }

  inline bool is_empty() {
    return (!buffer_->end_of_file() || buffer_->ask() > 0);
  }

protected:
  AsyncReadBuffer *buffer_;
  int batch_size_;                 // batch size
  int n_features_;                 // number of features in the dataset
  std::vector<Sample> batch_data_; // a batch of data samples
  std::set<Key> index_set_;        // the keys of features for the current batch
  Parse parse_;
};

} // namespace lib
} // namespace csci5570

