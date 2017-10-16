#pragma once

#include "base/abstract_partition_manager.hpp"
#include <algorithm>
#include "glog/logging.h"

namespace csci5570 {

class ConsistentHashingPartitionManager : public AbstractPartitionManager {
    public:
        ConsistentHashingPartitionManager(const std::vector<uint32_t>& ids) :
            AbstractPartitionManager(ids) {}

        size_t GetNumServers() const { return server_thread_ids_.size(); }

        const std::vector<uint32_t>& GetServerThreadIds() const { return server_thread_ids_; }

		void Slice(const Keys& keys, std::vector<std::pair<int, Keys>>* sliced) override{

			sliced->clear();
			
			for (auto key : keys){
				
				// Use JumpConsistentHash to generate the position, and take the node_id to be used in sliced.
				auto node_id = server_thread_ids_.at(JumpConsistentHash(key, server_thread_ids_.size()));
				DLOG(INFO) << "Key: "<< key << ", Node ID:" << node_id ;
			  
			  	// TO Construct the result set sliced : [ (0, [1,2,3] ), (1, [4,5,6])]
				// Find if the node id exists 
				auto it = std::find_if(sliced->begin(), sliced->end(), [&node_id](std::pair<int, Keys> ele) {return (ele.first == node_id);} );
				if ( it != sliced->end()){
					//Append to that node_id 
					it->second.append(Keys({key}));
					DLOG(INFO) << "APPEND "<< key << " to " << it->first ;					
				} else {
					sliced->push_back(std::make_pair(node_id, Keys({key})));				
					DLOG(INFO) << "ADD "<< key << " to " << node_id ;					
				}
				
			}
		}
		
        void Slice(const KVPairs& kvs, std::vector<std::pair<int, KVPairs>>* sliced) override{
			
            sliced->clear();
			
			for (auto i = 0 ; i < kvs.first.size(); i++){
				auto key = kvs.first[i];
				auto val = kvs.second[i];

				// Use JumpConsistentHash to generate the position, and take the node_id to be used in sliced.
				auto node_id = server_thread_ids_.at(JumpConsistentHash(key, server_thread_ids_.size()));
				DLOG(INFO) << "Key: "<< key << ", Node ID:" << node_id ;
				
			  	// TO Construct the result set sliced : [ (0, ([1,2,3],[.1,.2,.3]) ), (1, ([4,5,6], [.4,.5,.6])) ]
				// Find if the node id exists 
				auto it = std::find_if(sliced->begin(), sliced->end(), [&node_id](std::pair<int, KVPairs> ele) {return (ele.first == node_id);} );
				third_party::SArray<Key> keys({key});
				third_party::SArray<double> vals({val});

				if ( it != sliced->end()){					
					it->second.first.append(keys);
					it->second.second.append(vals);
					
					DLOG(INFO) << "APPEND "<< key  << ":" << val << " to " << it->first;
				} else {
					sliced->push_back(std::make_pair(node_id, std::make_pair(keys, vals) ));
					DLOG(INFO) << "ADD "<< key << " to " << node_id; 		
				}
				
			}
			
        }

    protected:
        Key est_max;

		// Jump Consistent Hash : https://arxiv.org/ftp/arxiv/papers/1406/1406.2294.pdf
		int32_t JumpConsistentHash(uint64_t key, int32_t num_buckets) {
		    int64_t b = -1, j = 0;
		    while (j < num_buckets) {
		        b = j;
		        key = key * 2862933555777941757ULL + 1;
		        j = (b + 1) * (double(1LL << 31) / double((key >> 33) + 1));
		    }
		    return b;
		}


};

} // namespace csci5560
