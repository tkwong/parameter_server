#pragma once

#include "glog/logging.h"

#include "base/abstract_partition_manager.hpp"
#include "base/magic.hpp"
#include "base/message.hpp"
#include "base/third_party/sarray.h"
#include "base/threadsafe_queue.hpp"
#include "worker/abstract_callback_runner.hpp"

#include <cinttypes>
#include <vector>

#include "gflags/gflags.h"
#include "glog/logging.h"

namespace csci5570 {

/**
 * Provides the API to users, and implements the worker-side abstraction of model
 * Each model in one application is uniquely handled by one KVClientTable
 *
 * @param Val type of model parameter values
 */
template <typename Val>
class KVClientTable {
 public:
  using Keys = third_party::SArray<Key>;
  using KVPairs = std::pair<third_party::SArray<Key>, third_party::SArray<double>>;
  /**
   * @param app_thread_id       user thread id
   * @param model_id            model id
   * @param sender_queue        the work queue of a sender communication thread
   * @param partition_manager   model partition manager
   * @param callback_runner     callback runner to handle received replies from servers
   */
  KVClientTable(uint32_t app_thread_id, uint32_t model_id, ThreadsafeQueue<Message>* const sender_queue,
                AbstractPartitionManager* const partition_manager, AbstractCallbackRunner* const callback_runner)
      : app_thread_id_(app_thread_id),
        model_id_(model_id),
        sender_queue_(sender_queue),
        partition_manager_(partition_manager),
        callback_runner_(callback_runner){};

  // ========== API ========== //
  void Clock()
  {
    CHECK_NOTNULL(partition_manager_);
    auto server_ids = partition_manager_->GetServerThreadIds();
    for (auto server_id : server_ids)
    {
      Message msg;
      msg.meta.sender = app_thread_id_;
      msg.meta.recver = server_id;
      msg.meta.model_id = model_id_;
      msg.meta.flag = Flag::kClock;
      sender_queue_->Push(msg);
    }
  }

  // vector version
  void Add(const std::vector<Key>& keys, const std::vector<Val>& vals)
  {
    Add(Keys(keys), third_party::SArray<Val>(vals)); // call the sarray version of Add()
  }

  void Get(const std::vector<Key>& keys, std::vector<Val>* vals)
  {
    third_party::SArray<Val> result;
    Get(Keys(keys), &result); // call the sarray version of Get()

    vals->clear();
    for (auto it=result.begin(); it!=result.end(); it++) vals->push_back(*it);
  }

  // sarray version
  void Add(const third_party::SArray<Key>& keys, const third_party::SArray<Val>& vals)
  {
    std::vector<std::pair<int, KVPairs>> sliced;
    partition_manager_->Slice<Val>(std::make_pair(keys, double_vals), &sliced);

    // Send Message to each server
    for (auto it=sliced.begin(); it!=sliced.end(); it++)
    {
        third_party::SArray<Val> v;
        for (auto vit=it->second.second.begin(); vit!=it->second.second.end(); vit++)
            v.push_back(*vit);

        Message msg;
        msg.meta.sender = app_thread_id_;
        msg.meta.recver = it->first;
        msg.meta.model_id = model_id_;
        msg.meta.flag = Flag::kAdd;

        msg.AddData(it->second.first);
        msg.AddData(v);

        sender_queue_->Push(msg);
    }
  }

  void Get(const third_party::SArray<Key>& keys, third_party::SArray<Val>* vals)
  {
    std::vector<std::pair<int, Keys>> sliced;
    partition_manager_->Slice(keys, &sliced);

    std::map<Key,Val> reply;

    // The callback will add the key-value pairs in the reply messages into a map
    callback_runner_->RegisterRecvHandle(app_thread_id_, model_id_,[&reply](Message& msg){
        auto re_keys = third_party::SArray<Key>(msg.data[0]);
        auto re_vals = third_party::SArray<Val>(msg.data[1]);

        for (int i=0; i<re_keys.size(); i++) reply.insert(std::make_pair(re_keys[i], re_vals[i]));
    });

    // TODO: We should have something to register in order to set the values to reply.
    callback_runner_->RegisterRecvFinishHandle(app_thread_id_, model_id_, []{});

    callback_runner_->NewRequest(app_thread_id_, model_id_, sliced.size());
    //LOG(INFO) << "Sending Messages To Servers";
    // Send request to each server
    for (auto it=sliced.begin(); it!=sliced.end(); it++)
    {
        Message msg;
        msg.meta.sender = app_thread_id_;
        msg.meta.recver = it->first;
        msg.meta.model_id = model_id_;
        msg.meta.flag = Flag::kGet;

        msg.AddData(it->second);

        sender_queue_->Push(msg);
    }
    //LOG(INFO) << "Wait Message from Servers";
    // wait request response
    callback_runner_->WaitRequest(app_thread_id_, model_id_);
    //LOG(INFO) << "Received Message from Servers";
    vals->clear();
    for (auto it=reply.begin(); it!=reply.end(); it++) vals->push_back(it->second);
  }
  // ========== API ========== //

 private:
  uint32_t app_thread_id_;  // identifies the user thread
  uint32_t model_id_;       // identifies the model on servers

  ThreadsafeQueue<Message>* const sender_queue_;             // not owned
  AbstractCallbackRunner* const callback_runner_;            // not owned
  AbstractPartitionManager* const partition_manager_;  // not owned

};  // class KVClientTable

}  // namespace csci5570
