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

namespace csci5570 {
namespace lib {

class AsyncReadBuffer {
public:
  using BatchT = std::vector<std::string>;
  ~AsyncReadBuffer() {
    thread_.join();
    master_thread_.join();
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
  void init(const std::string &url, int task_id, int num_threads,
            int batch_size, int batch_num) {
    
    if (init_) return ; // return if already init
    
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
        
    // zmq::context_t zmq_context(1);
    zmq_context = std::shared_ptr<zmq::context_t> (new zmq::context_t(1));
    // zmq::context_t zmq_context (1);
    // auto zmq_context = std::make_shared<zmq::context_t>(1);
    int proc_id = getpid();
    std::string master_host = "localhost";
    int master_port = 19817;  // use a random port number to avoid collision with other users
    std::string worker_host = "localhost";

    master_thread_ = std::thread([this, master_port, hdfs_namenode_port, hdfs_namenode] {
      HDFSBlockAssigner hdfs_block_assigner(hdfs_namenode, hdfs_namenode_port, zmq_context.get(), master_port);
      hdfs_block_assigner.Serve();
    });
    
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
              BatchT val({record.data()});
              buffer_.enqueue(val);
              LOG(INFO) << "Current Batch Count is " << batch_count_++;
          } else {
              eof_ = true;
              LOG(INFO) << "EOF Reached " ;
              break;
          }

        }

      } catch (const std::exception& e) {
         std::cout << e.what();
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
    if (eof_ && buffer_.size_approx() == 0) return false;
    // return buffer_.wait_dequeue(*batch);
    return buffer_.wait_dequeue_timed(*batch, std::chrono::milliseconds(5));
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
  moodycamel::BlockingConcurrentQueue<BatchT> buffer_;
  
  int batch_size_;      // the size of each batch
  int batch_num_;       // max buffered batch number
  int batch_count_ = 0; // unread buffered batch number
  int end_ = 0;         // writer appends to the end_
  int start_ = 0;       // reader reads from the start_

  // thread
  std::thread thread_;
  std::thread master_thread_;
  // std::mutex mutex_;
  // std::condition_variable load_cv_;
  // std::condition_variable get_cv_;
  
  bool init_ = false;
private: 
  
  
};

template <typename Sample> class AbstractAsyncDataLoader {
public:
  AbstractAsyncDataLoader(int batch_size, int num_features,
                          AsyncReadBuffer *buffer)
      : batch_size_(batch_size), n_features_(num_features), buffer_(buffer) {}

  virtual const std::vector<Sample> &get_data() {
    // parse data in buffer
    // return a batch of samples
  }
  std::vector<Key> get_keys() {
    // return the keys of features of the current batch
  }

  inline bool is_empty() {}

protected:
  AsyncReadBuffer *buffer_;
  int batch_size_;                 // batch size
  int n_features_;                 // number of features in the dataset
  std::vector<Sample> batch_data_; // a batch of data samples
  std::set<Key> index_set_;        // the keys of features for the current batch
};

} // namespace lib
} // namespace csci5570
