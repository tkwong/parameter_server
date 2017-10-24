#include "server/abstract_model.hpp"

#include "base/message.hpp"
#include "base/threadsafe_queue.hpp"
#include "server/abstract_storage.hpp"
#include "server/util/pending_buffer.hpp"
#include "server/util/progress_tracker.hpp"

#include <map>
#include <vector>

namespace csci5570 {

/**
 * A wrapper for model with Batch Synchronous Parallel consistency
 */
class BSPModel : public AbstractModel {
 public:
  explicit BSPModel(uint32_t model_id, std::unique_ptr<AbstractStorage>&& storage_ptr,
                    ThreadsafeQueue<Message>* reply_queue);

  virtual void Clock(Message& msg) override;
  virtual void Add(Message& msg) override;
  virtual void Get(Message& msg) override;
  virtual int GetProgress(int tid) override;
  virtual void ResetWorker(Message& msg) override;

  int GetGetPendingSize();
  int GetAddPendingSize();

 private:
  uint32_t model_id_;

  ThreadsafeQueue<Message>* reply_queue_;
  std::unique_ptr<AbstractStorage> storage_;
  ProgressTracker progress_tracker_;
  std::vector<Message> get_buffer_;  // buffer of get requests
  std::vector<Message> add_buffer_;  // buffer of add requests
};

}  // namespace csci5570
