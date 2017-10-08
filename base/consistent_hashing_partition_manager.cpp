#pragma once

#include "base/abstract_partition_manager.hpp"

namespace csci5570 {

class ConsistentHashingPartitionManager : public AbstractPartitionManager {
    public:
        ConsistentHashingPartitionManager(const std::vector<uint32_t>& ids) :
            AbstractPartitionManager(ids) {
              for (auto id : ids){
                AddNode(id);
              }
            }

        size_t GetNumServers() const { return server_thread_ids_.size(); }

        const std::vector<uint32_t>& GetServerThreadIds() const { return server_thread_ids_; }

        void Slice(const Keys& keys, std::vector<std::pair<int, Keys>>* sliced) {

            CHECK(!hash_ring_.empty());

            sliced->clear();

            for (auto key : keys){

              // Use Key_{id} as the hash value for choosing node_id
              std::string vkey_name = "Key_" + std::to_string(key);
              std::size_t hash_value = std::hash<std::string>{}(vkey_name);

              // always use upper_bound for searching the node_id, so that when any node removed, the partiation should not change too much.
              auto iter = hash_ring_.upper_bound(hash_value);
              if (iter == hash_ring_.end()) {
                iter = hash_ring_.begin();
              }
              LOG(INFO) << "Key: "<< key << ", Node ID:" << iter->second << " Hash:(" << hash_value << ")";
              sliced->push_back(std::make_pair(iter->second, Keys(key)));
            }
        }

        void Slice(const KVPairs& kvs, std::vector<std::pair<int, KVPairs>>* sliced) {
            CHECK(!hash_ring_.empty());
            sliced->clear();
            for (auto key : kvs.first){

              // Use Key_{id} as the hash value for choosing node_id
              std::string vkey_name = "Key_" + std::to_string(key);
              std::size_t hash_value = std::hash<std::string>{}(vkey_name);

              // always use upper_bound for searching the node_id, so that when any node removed, the partiation should not change too much.
              auto iter = hash_ring_.upper_bound(hash_value);
              if (iter == hash_ring_.end()) {
                iter = hash_ring_.begin();
              }
              LOG(INFO) << "Key: "<< key << ", Node ID:" << iter->second << " Hash:(" << hash_value << ")";
              sliced->push_back(std::make_pair(iter->second, KVPairs(key,kvs.first[key])));
            }
        }

        void AddNode(const uint32_t node_id){
          // Use Server_{id} as the hash value for the hash_ring_
          std::string vnode_name = "Server_" + std::to_string(node_id);
          std::size_t hash_value = std::hash<std::string>{}(vnode_name);
          hash_ring_[hash_value] = node_id;
          LOG(INFO) << "Add Node ID:" << node_id << " Hash:(" << hash_value << ")";
        }
        void RemoveNode(const uint32_t node_id){
          // Use Server_{id} as the hash value for the hash_ring_
          std::string vnode_name = "Server_" + std::to_string(node_id);
          std::size_t hash_value = std::hash<std::string>{}(vnode_name);
          hash_ring_.erase(hash_value);
          LOG(INFO) << "Remove Node ID:" << node_id << " Hash:(" << hash_value << ")";
        }
        void ClearNode(){
          hash_ring_.clear();
        }

    protected:
        Key est_max;
        std::map<uint32_t, uint32_t> hash_ring_;


};

} // namespace csci5560
