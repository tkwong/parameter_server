#pragma once

#include "base/range_partition_manager.hpp"
#include "server/map_storage.hpp"
#include "server/server_thread.cpp"

namespace csci5570 {

class TestModel : public AbstractModel {
    public:
        TestModel(AbstractStorage* storage, ThreadsafeQueue<Message>* reply_q) :
            storage(storage), reply_q(reply_q) {}
        virtual void Clock(Message&) override {}
        virtual void Add(Message& msg) override {storage->Add(msg);}
        virtual void Get(Message& msg) override {reply_q->Push(storage->Get(msg));}
        virtual int GetProgress(int tid) override {return 0;}
        virtual void ResetWorker(Message& msg) override {}
    
    protected:
        AbstractStorage* storage;
        ThreadsafeQueue<Message>* reply_q;
};

class SimpleServer {
    public:
        SimpleServer(int thread_num)
        {
            /*std::vector<uint32_t> ids;
            for (int i=0; i<thread_num; i++) ids.push_back(i);
            pm = new RangePartitionManager(ids);
            
            for (auto it=ids.begin(); it!=ids.end(); it++)
            {
                auto storage = new MapStorage<double>();
                std::unique_ptr<AbstractModel> model(new TestModel(storage, &reply_q));
                
                auto thread = new ServerThread(*it);
                thread->RegisterModel(0, std::move(model));
                threads.insert(std::make_pair(*it, thread));
            }*/
        }
        
        ThreadsafeQueue<Message>* getReplyQueue() {return &reply_q;}
        
    protected:
        AbstractPartitionManager<>* pm;
        std::map<uint32_t, ServerThread*> threads;
        ThreadsafeQueue<Message> reply_q;
};

} // namespace csci5570
