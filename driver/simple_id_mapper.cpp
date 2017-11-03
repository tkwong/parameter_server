#include "driver/simple_id_mapper.hpp"

#include <cinttypes>
#include <vector>
#include <algorithm>

#include "base/node.hpp"

namespace csci5570 {

SimpleIdMapper::SimpleIdMapper(Node node, const std::vector<Node>& nodes) {
    node_ = node;
    nodes_ = nodes;
}

uint32_t SimpleIdMapper::GetNodeIdForThread(uint32_t tid) {
    return tid / kMaxThreadsPerNode;
}

void SimpleIdMapper::Init(int num_server_threads_per_node) {
    if (num_server_threads_per_node < 1 || 
        num_server_threads_per_node >= kWorkerHelperThreadId) return;
  
    for (Node n:nodes_)
    {
        // Update node2server_
        auto server_id_vec = std::vector<uint32_t>();
        for (int i=kMaxThreadsPerNode*n.id; i<kMaxThreadsPerNode*n.id+num_server_threads_per_node;
            i++)
            server_id_vec.push_back(i);
        node2server_.insert(std::make_pair(n.id, server_id_vec));

        // Update node2worker_
        node2worker_helper_.insert(std::make_pair(n.id, std::vector<uint32_t>(
          {kMaxThreadsPerNode * n.id + kWorkerHelperThreadId})));
    }
}

uint32_t SimpleIdMapper::AllocateWorkerThread(uint32_t node_id) {
    if (std::find_if(nodes_.begin(), nodes_.end(), 
        [node_id](Node n)->bool{return n.id==node_id;}) == nodes_.end()) return -1;
    if (kMaxBgThreadsPerNode + node2worker_[node_id].size() >= kMaxThreadsPerNode) return -1;

    int new_id = kMaxThreadsPerNode*node_id + kMaxBgThreadsPerNode + 
        node2worker_[node_id].size();
    node2worker_[node_id].insert(new_id);

    return new_id;
}
void SimpleIdMapper::DeallocateWorkerThread(uint32_t node_id, uint32_t tid) {
    auto it = node2worker_.find(node_id);
    if (it == node2worker_.end()) return;
    auto set_it = it->second.find(tid);
    if (set_it == it->second.end()) return;
    it->second.erase(set_it);
}

std::vector<uint32_t> SimpleIdMapper::GetServerThreadsForId(uint32_t node_id) {
    auto it = node2server_.find(node_id);
    if (it != node2server_.end())
        return it->second;
    else
        return std::vector<uint32_t>();
}
std::vector<uint32_t> SimpleIdMapper::GetWorkerHelperThreadsForId(uint32_t node_id) {
    auto it = node2worker_helper_.find(node_id);
    if (it != node2worker_helper_.end())
        return it->second;
    else
        return std::vector<uint32_t>();
}
std::vector<uint32_t> SimpleIdMapper::GetWorkerThreadsForId(uint32_t node_id) {
    auto it = node2worker_.find(node_id);
    if (it != node2worker_.end())
        return std::vector<uint32_t>(it->second.begin(), it->second.end());
    else
        return std::vector<uint32_t>();
}
std::vector<uint32_t> SimpleIdMapper::GetAllServerThreads() {
    auto result = std::vector<uint32_t>();
    for (auto it=node2server_.begin(); it!=node2server_.end(); it++)
        result.insert(result.end(), it->second.begin(), it->second.end());
    return result;
}

const uint32_t SimpleIdMapper::kMaxNodeId;
const uint32_t SimpleIdMapper::kMaxThreadsPerNode;
const uint32_t SimpleIdMapper::kMaxBgThreadsPerNode;
const uint32_t SimpleIdMapper::kWorkerHelperThreadId;

}  // namespace csci5570
