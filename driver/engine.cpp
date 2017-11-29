#include "driver/engine.hpp"

#include <vector>

#include "base/abstract_partition_manager.hpp"
#include "base/range_partition_manager.hpp"
#include "base/node.hpp"
#include "comm/mailbox.hpp"
#include "comm/sender.hpp"
#include "driver/ml_task.hpp"
#include "driver/simple_id_mapper.hpp"
#include "driver/worker_spec.hpp"
#include "server/server_thread.hpp"
#include "worker/abstract_callback_runner.hpp"
#include "worker/callback_runner.cpp"
#include "worker/worker_thread.hpp"

#include "glog/logging.h"

namespace csci5570 {

class UserThread : public AbstractWorkerThread
{
    public:
        UserThread(uint32_t id, const MLTask& task, Info info) : 
            AbstractWorkerThread(id), task(task), info(info) {}
    protected:
        virtual void OnReceive(Message& msg) override {}
        virtual void Main() override
        {
            task.RunLambda(info);
        }

        const MLTask& task;
        Info info;
};

class WorkerHelperThread : public AbstractWorkerThread
{
    public:
        WorkerHelperThread(uint32_t id, AbstractCallbackRunner* runner) : 
            AbstractWorkerThread(id), runner(runner) {}
    protected:
        virtual void OnReceive(Message& msg) override
        {
            runner->AddResponse(msg.meta.recver, msg.meta.model_id, msg);
        }
        virtual void Main() override
        {
            while (true)
            {
                Message msg;
                work_queue_.WaitAndPop(&msg);

                if (msg.meta.flag == Flag::kExit) break;

                OnReceive(msg);
            }
        }

        AbstractCallbackRunner* runner; 
};

void Engine::StartEverything(int num_server_threads_per_node) {
    CreateIdMapper(num_server_threads_per_node);
    CreateMailbox();

    StartSender();

    for (auto server_id : id_mapper_.get()->GetServerThreadsForId(node_.id))
        server_thread_group_.push_back(new ServerThread(server_id));

    uint32_t worker_id = id_mapper_.get()->AllocateWorkerThread(node_.id);
    callback_runner_ = std::unique_ptr<AbstractCallbackRunner>(new CallbackRunner());
    worker_thread_ = std::unique_ptr<AbstractWorkerThread>(
        new WorkerHelperThread(worker_id, callback_runner_.get()));

    for (auto server_ptr : server_thread_group_)
    {
        LOG(INFO) << "Registering Server id: " << server_ptr->GetId() << " to mailbox";
        mailbox_.get()->RegisterQueue(server_ptr->GetId(), server_ptr->GetWorkQueue());
    }
    mailbox_.get()->RegisterQueue(worker_thread_.get()->GetId(), 
                                  worker_thread_.get()->GetWorkQueue());
    
    StartServerThreads();
    StartWorkerThreads();

    StartMailbox();
}
void Engine::CreateIdMapper(int num_server_threads_per_node) {
    id_mapper_ = std::unique_ptr<SimpleIdMapper>(new SimpleIdMapper(node_, nodes_));
    id_mapper_.get()->Init(num_server_threads_per_node);
    LOG(INFO) << "id_mapper_ created";
}
void Engine::CreateMailbox() {
    mailbox_ = std::unique_ptr<Mailbox>(new Mailbox(node_, nodes_, id_mapper_.get()));
    sender_ = std::unique_ptr<Sender>(new Sender(mailbox_.get()));
    LOG(INFO) << "mailbox_ and sender_ created";
}
void Engine::StartServerThreads() {
    for (auto it=server_thread_group_.begin(); it!=server_thread_group_.end(); it++)
        (*it)->Start();
    LOG(INFO) << "server threads started";
}
void Engine::StartWorkerThreads() {
    worker_thread_.get()->Start();
    LOG(INFO) << "worker threads started";
}
void Engine::StartMailbox() {
    mailbox_.get()->Start();
    LOG(INFO) << "Mailbox started";
}
void Engine::StartSender() {
    sender_.get()->Start();
    LOG(INFO) << "Sender started";
}

void Engine::StopEverything() {
    StopSender();
    Barrier();
    StopMailbox();
    StopServerThreads();
    StopWorkerThreads();
}
void Engine::StopServerThreads() {
    for (auto it=server_thread_group_.begin(); it!=server_thread_group_.end(); it++)
        (*it)->Stop();
    LOG(INFO) << "Server threads stopped";
}
void Engine::StopWorkerThreads() {
    worker_thread_.get()->Stop();
    LOG(INFO) << "Worker threads stopped";
}
void Engine::StopSender() {
    sender_.get()->Stop();
    LOG(INFO) << "Sender stopped";
}
void Engine::StopMailbox() {
    mailbox_.get()->Stop();
    LOG(INFO) << "Mailbox stopped";
}

void Engine::Barrier() {
    mailbox_.get()->Barrier();
    LOG(INFO) << "Barrier called";
}

WorkerSpec Engine::AllocateWorkers(const std::vector<WorkerAlloc>& worker_alloc) {
    WorkerSpec result(worker_alloc);
    auto workers = result.GetLocalWorkers(node_.id);
    LOG(INFO) << "workers size: " << workers.size();
    for (auto worker : workers)
    {
        uint32_t tid = id_mapper_.get()->AllocateWorkerThread(node_.id);
        result.InsertWorkerIdThreadId(worker, tid);
    }
    return result;
}

void Engine::InitTable(uint32_t table_id, const std::vector<uint32_t>& worker_ids) {
    //LOG(INFO) << "Initializing table with worker_ids: ";
    //for (auto worker_id : worker_ids) LOG(INFO) << worker_id;
    //LOG(INFO) << "---------------------------------------------------";

    Message terminate;
    terminate.meta.flag = Flag::kExit;
    worker_thread_.get()->GetWorkQueue()->Push(terminate);
    worker_thread_.get()->Stop();

    for (auto server_id : id_mapper_.get()->GetAllServerThreads())
    {
        // auto model_ptr = server_ptr->GetModel(table_id);
        // if (!model_ptr) continue;

        LOG(INFO) << "Sending Reset Message to server_id: " << server_id << " for table_id: " << table_id;
        Message reset_msg;
        reset_msg.meta.sender = worker_thread_.get()->GetId();
        reset_msg.meta.recver = server_id;
        reset_msg.meta.flag = Flag::kResetWorkerInModel;
        reset_msg.meta.model_id = table_id;
        reset_msg.AddData(third_party::SArray<uint32_t>(worker_ids));
        sender_.get()->GetMessageQueue()->Push(reset_msg);

        Message reply;
        worker_thread_.get()->GetWorkQueue()->WaitAndPop(&reply);
        LOG(INFO) << "Reply message received";
    }

    worker_thread_.get()->Start();
}

void Engine::Run(const MLTask& task) {
    CHECK(task.IsSetup());
    
    WorkerSpec spec = AllocateWorkers(task.GetWorkerAlloc());
    LOG(INFO) << "Allocated Threads size " << spec.GetLocalThreads(node_.id).size();;
    
    // Init tables
    for (uint32_t table_id : task.GetTables())
        InitTable(table_id, spec.GetAllThreadIds());
    Barrier();

    // Create and Initialize TimeTable and WorkloadTable
    std::vector<third_party::Range> ranges;
    for (auto node :  nodes_)
        ranges.push_back(third_party::Range(node.id, node.id + 1)); // end range is exclusive

    auto* time_pm = new RangePartitionManager(id_mapper_.get()->GetAllServerThreads(), ranges);
    auto timeTable_id = CreateTable<double>(std::unique_ptr<AbstractPartitionManager>(time_pm),
        ModelType::BSP, StorageType::Map);
    InitTable(timeTable_id, spec.GetAllThreadIds());
    Barrier();

    auto* work_pm = new RangePartitionManager(id_mapper_.get()->GetAllServerThreads(), ranges);
    auto workloadTable_id = CreateTable<int>(std::unique_ptr<AbstractPartitionManager>(work_pm),
        ModelType::ASP, StorageType::Map);
    InitTable(workloadTable_id, spec.GetAllThreadIds());
    Barrier();
    
    // Spawn user threads if server == workers
    if (spec.HasLocalWorkers(node_.id)) {
      std::vector<UserThread*> workers;
      auto loc_workers = spec.GetLocalWorkers(node_.id);
      auto loc_threads  = spec.GetLocalThreads(node_.id);
      for (int i=0; i<loc_threads.size(); i++)
      {
          auto pm_map = new std::map<uint32_t, AbstractPartitionManager*>();
          for (auto it = partition_manager_map_.begin(); it!=partition_manager_map_.end(); it++)
              pm_map->insert(std::make_pair(it->first, it->second.get()));

          Info info;
          info.thread_id = loc_threads[i];
          info.worker_id = loc_workers[i];
          info.send_queue = sender_.get()->GetMessageQueue();
          info.partition_manager_map = *pm_map;
          info.callback_runner = callback_runner_.get();
          info.timeTable_id = timeTable_id;
          info.workloadTable_id = workloadTable_id;

          LOG(INFO) << "thread_id: " << info.thread_id << " worker_id: " << info.worker_id
                    << " timeTable_id: " << info.timeTable_id << " workloadTable_id: "
                    << info.workloadTable_id;

          UserThread* worker = new UserThread(info.thread_id, task, info);
          mailbox_.get()->RegisterQueue(info.thread_id, worker_thread_.get()->GetWorkQueue());
          workers.push_back(worker);
          worker->Start();
      }
      for (auto worker : workers) worker->Stop();      
    }


}

void Engine::RegisterPartitionManager(uint32_t table_id, 
    std::unique_ptr<AbstractPartitionManager> partition_manager) 
{
    partition_manager_map_.insert(std::make_pair(table_id, std::move(partition_manager)));
}

}  // namespace csci5570
