#pragma once

#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "boost/tokenizer.hpp"

#include "base/magic.hpp"
#include "io/line_input_format.hpp"
#include "lib/labeled_sample.hpp"

namespace csci5570 {
namespace lib {

class AsyncReadBuffer {
public:
  using BatchT = std::vector<std::string>;
  ~AsyncReadBuffer() {}

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
    // 1. initialize input format
                
    buffer_ = std::vector<BatchT> (batch_num);
    std::string hdfs_namenode = "proj10";
    int hdfs_namenode_port = 9000;
    int master_port = 19817;  // use a random port number to avoid collision with other users
    zmq::context_t zmq_context(1);

    int proc_id = getpid();
    std::string master_host = "proj10";
    std::string worker_host = "proj10";

    Coordinator coordinator(proc_id, worker_host, &zmq_context, master_host,
                            master_port);
    coordinator.serve();
    DLOG(INFO) << "Coordinator begins serving";

    auto infmt_ =
        new LineInputFormat(url, num_threads, task_id, &coordinator,
                            worker_host, hdfs_namenode, hdfs_namenode_port);

    DLOG(INFO) << "Line input is well prepared";
    // 2. spawn spreads to asynchronously load data
    for (int n = 0 ; n < num_threads; n++) {
        
      std::thread t([n,infmt_,this]() {
        
        DLOG(INFO) << "thread #" << std::to_string(n) << " id:" << std::this_thread::get_id() << " started\n";

        // put the line into the batch.
        bool success = true;
        boost::string_ref record;
        while (true) {
          success = infmt_->next(record);
          if (success != true) {
            break;              
          } else {
            BatchT val({record.data()});
            buffer_.push_back(val);  
          }
            
        }
        
        DLOG(INFO) << "thread #" << n << " id:" << std::this_thread::get_id() << " ended\n";
      });
    }
  }

  bool get_batch(BatchT *batch) {
    // store batch_size_ records in <batch> and return true if success
    // i.e. Consumer
    if (buffer_.size() > 0) {
      // batch = &(buffer_.pop_back()); // FIXME: pop_front instead?
      return true;
    } else {
      return false;
    }
  }

  int ask() {
    // return the number of batches buffered
    buffer_.size();
  }

  inline bool end_of_file() const {
    // return true if the end of the file is reached
    return eof_;
  }

protected:
  virtual void main() {
    // the workloads of the thread reading samples asynchronously
  }

  // input
  std::unique_ptr<LineInputFormat> infmt_;
  std::atomic<bool> eof_{false};

  // buffer
  std::vector<BatchT> buffer_;
  int batch_size_;      // the size of each batch
  int batch_num_;       // max buffered batch number
  int batch_count_ = 0; // unread buffered batch number
  int end_ = 0;         // writer appends to the end_
  int start_ = 0;       // reader reads from the start_

  // thread
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable load_cv_;
  std::condition_variable get_cv_;
  bool init_ = false;
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
