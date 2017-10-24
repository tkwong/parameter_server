#pragma once

#include <cinttypes>
#include <vector>

#include "base/abstract_partition_manager.hpp"
#include "base/magic.hpp"
#include "base/third_party/sarray.h"

#include "glog/logging.h"

namespace csci5570 {

class RangePartitionManager : public AbstractPartitionManager {
    public:
        RangePartitionManager(const std::vector<uint32_t>& server_thread_ids,
            const std::vector<third_party::Range>& ranges) :
            AbstractPartitionManager(server_thread_ids),  ranges_(ranges) {}
        void Slice(const Keys& keys, std::vector<std::pair<int, Keys>>* sliced) override
        {
            sliced->clear();
            int range = 0, server = -1;

            for (auto k_it=keys.begin(); k_it!=keys.end(); k_it++)
            {
                if((*k_it >= ranges_[range].begin() && *k_it < ranges_[range].end()) ||
                    range+1 >= ranges_.size()) // Within range or out of the last range
                {
                    if (server < range) // Pair not exists
                    {
                        server = range;
                        sliced->push_back(std::make_pair(
                            server_thread_ids_[server], Keys({*k_it})));
                    }
                    else
                    {
                        sliced->back().second.push_back(*k_it);
                    }
                }
                else
                {
                    range++;
                    k_it--;
                }
            }
        }

        void Slice(const KVPairs& kvs, std::vector<std::pair<int, KVPairs>>* sliced) override
        {
            sliced->clear();
            int range = 0, server = -1;

            for (int i=0; i<kvs.first.size(); i++)
            {
                if((kvs.first[i] >= ranges_[range].begin() && kvs.first[i] < ranges_[range].end())
                    || range+1 >= ranges_.size()) // Within range or out of the last range
                {
                    if (server < range) // Pair not exists
                    {
                        server = range;
                        sliced->push_back(std::make_pair(
                            server_thread_ids_[server],
                            KVPairs({kvs.first[i]},{kvs.second[i]})));
                    }
                    else
                    {
                        sliced->back().second.first.push_back(kvs.first[i]);
                        sliced->back().second.second.push_back(kvs.second[i]);
                    }
                }
                else
                {
                    range++;
                    i--;
                }
            }
        }
    private:
        std::vector<third_party::Range> ranges_;
};

}  // namespace csci5570
