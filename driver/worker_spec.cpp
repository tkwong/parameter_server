#include "driver/worker_spec.hpp"
#include "glog/logging.h"

namespace csci5570 {

WorkerSpec::WorkerSpec(const std::vector<WorkerAlloc>& worker_alloc) {
    Init(worker_alloc);
}

bool WorkerSpec::HasLocalWorkers(uint32_t node_id) const {
    return node_to_workers_.at(node_id).size() > 0;
}

const std::vector<uint32_t>& WorkerSpec::GetLocalWorkers(uint32_t node_id) const {
    auto it = node_to_workers_.find(node_id);
    if (it != node_to_workers_.end())
        return it->second;
    else
        return *(new std::vector<uint32_t>());
}

const std::vector<uint32_t>& WorkerSpec::GetLocalThreads(uint32_t node_id) const {
    auto it = node_to_threads_.find(node_id);
    if (it != node_to_threads_.end())
        return it->second;
    else
        return *(new std::vector<uint32_t>());
}

std::map<uint32_t, std::vector<uint32_t>> WorkerSpec::GetNodeToWorkers() {
    return node_to_workers_;
}

std::vector<uint32_t> WorkerSpec::GetAllThreadIds() {
    return std::vector<uint32_t>(thread_ids_.begin(), thread_ids_.end());
}

void WorkerSpec::InsertWorkerIdThreadId(uint32_t worker_id, uint32_t thread_id) {
    auto nid = worker_to_node_.at(worker_id);

    worker_to_thread_.insert(std::make_pair(worker_id, thread_id));
    thread_to_worker_.insert(std::make_pair(thread_id, worker_id));

    node_to_threads_[nid].push_back(thread_id);
    thread_ids_.insert(thread_id);
}

void WorkerSpec::Init(const std::vector<WorkerAlloc>& worker_alloc) {
    for (WorkerAlloc w_alloc : worker_alloc)
    {
        std::vector<uint32_t> worker_ids;
        for (int i=0; i<w_alloc.num_workers; i++)
        {
            worker_to_node_.insert(std::make_pair(num_workers_, w_alloc.node_id));
            worker_ids.push_back(num_workers_++);
        }
        node_to_workers_.insert(std::make_pair(w_alloc.node_id, worker_ids));
    }
}
}  // namespace csci5570
