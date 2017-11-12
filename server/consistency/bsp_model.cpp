#include "server/consistency/bsp_model.hpp"
#include "glog/logging.h"
#include "memory"

namespace csci5570 {

BSPModel::BSPModel(uint32_t model_id, std::unique_ptr<AbstractStorage>&& storage_ptr,
                   ThreadsafeQueue<Message>* reply_queue) {
  model_id_ = model_id;
  reply_queue_ = reply_queue;
  storage_ = std::move(storage_ptr);
}

void BSPModel::Clock(Message& msg) {
  const int clock = progress_tracker_.AdvanceAndGetChangedMinClock(msg.meta.sender);
  // DLOG(INFO) << "CLOCKED " << clock << " msg.meta.sender " << msg.meta.sender;
  if (clock != -1)
  {
      for(auto message : get_buffer_) {
        this->Get(message);
      }
      get_buffer_.clear();

      for(auto message : add_buffer_) {
        // this->Add(message);
        // CHANGED: now do the actual storage Add
        storage_->Add(message);
      } 
      add_buffer_.clear();
  }
}

void BSPModel::Add(Message& msg) {
  // CHANGED: it seems the add can be done when all worker clocked. 
  // const int clock = progress_tracker_.GetProgress(msg.meta.sender);
  // DLOG(INFO) << "ADD sender: " << msg.meta.sender << " clock: " << clock  << " MinClock : " << progress_tracker_.GetMinClock() ;
  //
  // if ( clock > progress_tracker_.GetMinClock() + 1)
  // {
  //   DLOG(INFO) << "ADD to buffer";
  //   add_buffer_.push_back(msg);
  // }
  // else
  // {
  //   DLOG(INFO) << "Add to Storage";
  //   storage_->Add(msg);
  // }
  add_buffer_.push_back(msg);
}

void BSPModel::Get(Message& msg) {
  const int clock = progress_tracker_.GetProgress(msg.meta.sender);
  // DLOG(INFO) << "Get sender: " << msg.meta.sender << " clock: " << clock ;
  if (clock > progress_tracker_.GetMinClock() + 1)
      get_buffer_.push_back(msg);
  else
      reply_queue_->Push(storage_->Get(msg));  
}

int BSPModel::GetProgress(int tid) {
  return progress_tracker_.GetProgress(tid);
}

int BSPModel::GetGetPendingSize() {
  return get_buffer_.size();
}

int BSPModel::GetAddPendingSize() {
  return add_buffer_.size();;
}

void BSPModel::ResetWorker(Message& msg) {
  third_party::SArray<uint32_t> tids(msg.data[0]);
  DLOG(INFO) << "Init progress_tracker " << tids;
  progress_tracker_.Init(std::vector<uint32_t>(tids.begin(), tids.end()));


  Message reply_msg;
  reply_msg.meta.model_id = model_id_;
  reply_msg.meta.sender = msg.meta.recver;
  reply_msg.meta.recver = msg.meta.sender;
  reply_msg.meta.flag = Flag::kResetWorkerInModel;
  
  reply_queue_->Push(reply_msg);
}

}  // namespace csci5570
