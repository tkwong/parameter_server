#include "server/consistency/ssp_model.hpp"
#include "glog/logging.h"

namespace csci5570 {

SSPModel::SSPModel(uint32_t model_id, std::unique_ptr<AbstractStorage>&& storage_ptr, 
    int staleness, ThreadsafeQueue<Message>* reply_queue) 
{
    model_id_ = model_id;
    staleness_ = staleness;
    reply_queue_ = reply_queue;
    storage_ = std::move(storage_ptr);
}

void SSPModel::Clock(Message& msg) {
    const int new_min = progress_tracker_.AdvanceAndGetChangedMinClock(msg.meta.sender);

    if (new_min != -1)
    {
        auto messages = buffer_.Pop(new_min);
        for(auto message : messages) reply_queue_->Push(message);
    }
}

void SSPModel::Add(Message& msg) {
    storage_->Add(msg);
}

void SSPModel::Get(Message& msg) {
    const int clock = progress_tracker_.GetProgress(msg.meta.sender);

    if (clock > progress_tracker_.GetMinClock() + staleness_)
        buffer_.Push(clock - staleness_, msg);
    else
        reply_queue_->Push(storage_->Get(msg));
}

int SSPModel::GetProgress(int tid) {
    return progress_tracker_.GetProgress(tid);
}

int SSPModel::GetPendingSize(int progress) {
    return buffer_.Size(progress);
}

void SSPModel::ResetWorker(Message& msg) {
    LOG(INFO) << "Received Reset Message";
    third_party::SArray<uint32_t> tids(msg.data[0]);
    progress_tracker_.Init(std::vector<uint32_t>(tids.begin(), tids.end()));

    Message reply;
    reply.meta.sender = msg.meta.recver;
    reply.meta.recver = msg.meta.sender;
    reply.meta.flag = Flag::kResetWorkerInModel;
    reply_queue_->Push(reply);
    LOG(INFO) << "Reply message sent";
}

}  // namespace csci5570
