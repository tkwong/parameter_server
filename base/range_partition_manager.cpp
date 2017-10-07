#pragma once

#include "base/abstract_partition_manager.hpp"

namespace csci5570 {

class RangePartitionManager : public AbstractPartitionManager {
    public:
        RangePartitionManager(const std::vector<uint32_t>& ids, Key est_max = 100) :
            AbstractPartitionManager(ids), est_max(est_max) {}
        
        size_t GetNumServers() const { return server_thread_ids_.size(); }
        const std::vector<uint32_t>& GetServerThreadIds() const { return server_thread_ids_; }
        
        void Slice(const Keys& keys, std::vector<std::pair<int, Keys>>* sliced) {
            sliced->clear();
            for (auto it=server_thread_ids_.begin(); it!=server_thread_ids_.end(); it++)
                sliced->push_back(std::make_pair(*it, Keys()));
                
            Keys sorted_keys(keys);
            std::sort(sorted_keys.begin(), sorted_keys.end());
            
            int i=0;
            for (auto it=sorted_keys.begin(); it!=sorted_keys.end(); it++)
            {
                Key key = *it;
                if (key < float(est_max) / server_thread_ids_.size() * (i+1) || 
                    i+1 >= server_thread_ids_.size())
                {
                    sliced->at(i).second.push_back(key);
                }
                else
                {
                    i++;
                    it--;
                }
            }
        }
        
        void Slice(const KVPairs& kvs, std::vector<std::pair<int, KVPairs>>* sliced) {
            sliced->clear();
            for (auto it=server_thread_ids_.begin(); it!=server_thread_ids_.end(); it++)
                sliced->push_back(std::make_pair(*it, KVPairs()));
                
            std::map<Key,double> sorted_kvs;
            for (int i=0; i<kvs.first.size(); i++)
                sorted_kvs.insert(std::make_pair(kvs.first[i], kvs.second[i]));
            
            int i=0;
            for (auto it=sorted_kvs.begin(); it!=sorted_kvs.end(); it++)
            {
                Key key = it->first;
                double value = it->second;
                
                if (key < float(est_max) / server_thread_ids_.size() * (i+1) || 
                    i+1 >= server_thread_ids_.size())
                {
                    sliced->at(i).second.first.push_back(key);
                    sliced->at(i).second.second.push_back(value);
                }
                else
                {
                    i++;
                    it--;
                }
            }
        }
        
    protected:
        Key est_max;
};

} // namespace csci5560
