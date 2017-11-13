#pragma once

#include <condition_variable>
#include <mutex>

#include "abstract_callback_runner.hpp"

namespace csci5570 {

class CallbackRunner: public AbstractCallbackRunner {
    public:
        CallbackRunner() {}
        
        void RegisterRecvHandle(uint32_t app_thread_id, uint32_t model_id, 
            const std::function<void(Message&)>& recv_handle) override
        {
            std::unique_lock<std::mutex> lk(mu_);
            recv_handles[std::make_pair(app_thread_id, model_id)] = recv_handle;
        }
        
        void RegisterRecvFinishHandle(uint32_t app_thread_id, uint32_t model_id,
            const std::function<void()>& recv_finish_handle) override
        {
            std::unique_lock<std::mutex> lk(mu_);
            recv_finish_handles[std::make_pair(app_thread_id, model_id)] = recv_finish_handle;
        }
        
        void NewRequest(uint32_t app_thread_id, uint32_t model_id, 
            uint32_t expected_responses) override
        {
            std::unique_lock<std::mutex> lk(mu_);
            trackers[std::make_pair(app_thread_id, model_id)] = 
                new std::pair<uint32_t,uint32_t>({expected_responses, 0});
        }
        
        void WaitRequest(uint32_t app_thread_id, uint32_t model_id) override
        {
            std::unique_lock<std::mutex> lk(mu_);
            cond_.wait(lk, [this, app_thread_id, model_id] { 
              auto tracker = trackers[std::make_pair(app_thread_id, model_id)];
              return tracker->first == tracker->second; 
            });
        }
        
        void AddResponse(uint32_t app_thread_id, uint32_t model_id, Message& msg) override
        {
            auto tracker = trackers[std::make_pair(app_thread_id, model_id)];
            auto recv_handle = recv_handles[std::make_pair(app_thread_id, model_id)];
            auto recv_finish_handle = recv_finish_handles[std::make_pair(app_thread_id, model_id)];
            
            //LOG(INFO) << "tracker:" << tracker->first << " " << tracker->second;
            
            bool recv_finish = false;
            {
              std::lock_guard<std::mutex> lk(mu_);
              recv_finish = tracker->first == tracker->second + 1 ? true : false;
            }
            
            // LOG(INFO) << "recv_finish: " << (recv_finish==true?"True":"False");
            
            recv_handle(msg);
            if (recv_finish) recv_finish_handle();
            
            {
              std::lock_guard<std::mutex> lk(mu_);
              tracker->second += 1;
              if (recv_finish) cond_.notify_all();
            }
        } 
    private:
        std::map<std::pair<uint32_t,uint32_t>, std::function<void(Message&)>> recv_handles;
        std::map<std::pair<uint32_t,uint32_t>, std::function<void()>> recv_finish_handles;

        std::mutex mu_;
        std::condition_variable cond_;
        std::map<std::pair<uint32_t,uint32_t>, std::pair<uint32_t,uint32_t>*> trackers;
}; // class CallbackRunner

} //namespace csci5570
