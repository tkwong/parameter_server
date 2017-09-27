#include "server/server_thread.hpp"

#include "glog/logging.h"

namespace csci5570 {

void ServerThread::RegisterModel(uint32_t model_id, std::unique_ptr<AbstractModel>&& model) {
  models_.insert(std::make_pair(model_id, std::move(model)));
}

AbstractModel* ServerThread::GetModel(uint32_t model_id) {
  auto it = models_.find(model_id);
  if (it != models_.end()){
     return it->second.get();
   } else {
     return nullptr;
   }
}

void ServerThread::Main() {
  while(true){
    Message msg;
    work_queue_.WaitAndPop(&msg);
    
    if(msg.meta.flag == Flag::kExit) break;
    
    AbstractModel* model = GetModel(msg.meta.model_id);
    if (model == nullptr) continue; //Skip this message if the model_id does not exist.

    switch(msg.meta.flag){
      case Flag::kClock:
        model->Clock(msg);
        break;
      case Flag::kAdd:
        model->Add(msg);
        break;
      case Flag::kGet:
        model->Get(msg);
        break;
    }
}

}  // namespace csci5570

