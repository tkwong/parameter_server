#pragma once

#include <cinttypes>
#include <utility>
#include <vector>

#include "base/magic.hpp"
#include "base/third_party/sarray.h"

namespace csci5570 {

/*
 * Implments the interface of a PartitionManager which provides the model
 * partitioning scheme
 */
template <typename Val = double>
class AbstractPartitionManager {
public:
  // using Keys = std::vector<Key>;
  // using KVPairs = std::pair<std::vector<Key>, std::vector<double>>;
  typedef third_party::SArray<Key> Keys;
  typedef std::pair<third_party::SArray<Key>, third_party::SArray<Val>> KVPairs;
  
  explicit AbstractPartitionManager(
      const std::vector<uint32_t> &server_thread_ids)
      : server_thread_ids_(server_thread_ids) {}

  size_t GetNumServers() const {
    return server_thread_ids_.size();
  }
  const std::vector<uint32_t> &GetServerThreadIds() const
  {
    return server_thread_ids_;
  }

  // slice keys into <server_id, key_partition> pairs
  virtual void Slice(const Keys &keys,
                     std::vector<std::pair<int, Keys>> *sliced) const = 0;
  // slice key-value pairs into <server_id, key_value_partition> pairs
  virtual void Slice(const KVPairs &kvs,
                     std::vector<std::pair<int, KVPairs>> *sliced) const = 0;

protected:
  std::vector<uint32_t> server_thread_ids_;
};  // class AbstractPartitionManager

}  // namespace csci5570
