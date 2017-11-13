#include "server/consistency/asp_model.hpp"
#include "glog/logging.h"

namespace csci5570 {

ASPModel::ASPModel(uint32_t model_id, std::unique_ptr<AbstractStorage>&& storage_ptr,
                   ThreadsafeQueue<Message>* reply_queue) {
 model_id_ = model_id;
 reply_queue_ = reply_queue;
 storage_ = std::move(storage_ptr);

}

void ASPModel::Clock(Message& msg) {
  // noop
}

void ASPModel::Add(Message& msg) {
  storage_->Add(msg);
}

void ASPModel::Get(Message& msg) {
  reply_queue_->Push(storage_->Get(msg));  
}

int ASPModel::GetProgress(int tid) {
  return progress_tracker_.GetProgress(tid);
}

void ASPModel::ResetWorker(Message& msg) {
  third_party::SArray<uint32_t> tids(msg.data[0]);
  progress_tracker_.Init(std::vector<uint32_t>(tids.begin(), tids.end()));

  Message reply_msg;
  reply_msg.meta.model_id = model_id_;
  reply_msg.meta.sender = msg.meta.recver;
  reply_msg.meta.recver = msg.meta.sender;
  reply_msg.meta.flag = Flag::kResetWorkerInModel;
  
  reply_queue_->Push(reply_msg);
}

}  // namespace csci5570
