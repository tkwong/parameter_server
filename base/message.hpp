#pragma once

#include <cinttypes>
#include <sstream>

#include "base/magic.hpp"
#include "base/serialization.hpp"
#include "base/third_party/sarray.h"

namespace csci5570 {

struct Control {};

enum class Flag : char { kExit, kBarrier, kResetWorkerInModel, kClock, kAdd, kGet };
static const char* FlagName[] = {"kExit", "kBarrier", "kResetWorkerInModel", "kClock", "kAdd", "kGet"};

struct Meta {
  Flag flag;  // {kExit, kBarrier, kResetWorkerInModel, kClock, kAdd, kGet}
  int sender;
  int recver;
  int model_id;

  std::string DebugString() const {
    std::stringstream ss;
    ss << "Meta: { ";
    ss << "sender: " << sender;
    ss << ", recver: " << recver;
    if( flag != Flag::kBarrier && flag != Flag::kExit )
      ss << ", model_id: " << model_id;
    ss << ", flag: " << FlagName[static_cast<int>(flag)];

    ss << "}";
    return ss.str();
  }
  // Initialize with -1
  Meta() : sender(-1), recver(-1), model_id(-1) {}
};

struct Message {
  Meta meta;
  std::vector<third_party::SArray<char>> data;

  template <typename V>
  void AddData(const third_party::SArray<V>& val) {
    data.push_back(third_party::SArray<char>(val));
  }

  std::string DebugString() const {
    std::stringstream ss;
    ss << meta.DebugString();
    if (data.size()) {
      ss << " Body:";
      for (const auto& d : data)
        ss << " data_size=" << d.size();
    }
    return ss.str();
  }
};

}  // namespace csci5570
