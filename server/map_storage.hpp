#pragma once

#include "base/message.hpp"
#include "server/abstract_storage.hpp"

#include "glog/logging.h"

#include <map>

namespace csci5570 {

template <typename Val>
class MapStorage : public AbstractStorage {
 public:
  MapStorage() = default;

  virtual void SubAdd(const third_party::SArray<Key>& typed_keys, 
      const third_party::SArray<char>& vals) override {
    auto typed_vals = third_party::SArray<Val>(vals);
    CHECK_EQ(typed_keys.size(), typed_vals.size());

    for (int i=0; i<typed_keys.size(); i++)
      storage_[typed_keys[i]] = typed_vals[i];

    //for (int i=0; i<typed_keys.size(); i++)
    //    LOG(INFO) << "Add Key: " << typed_keys[i] << " Val:" << typed_vals[i];
  }

  virtual third_party::SArray<char> SubGet(const third_party::SArray<Key>& typed_keys) override {
    third_party::SArray<Val> reply_vals(typed_keys.size());

    for (int i=0; i<typed_keys.size(); i++) {
      auto it = storage_.find(typed_keys[i]);
      if (it != storage_.end())
        reply_vals[i] = it->second;
      else
        break; //If a key does not exist, return the values founded so far.
    }

    //for (int i=0; i<typed_keys.size(); i++)
    //    LOG(INFO) << "Get Key: " << typed_keys[i] << " Val:" << reply_vals[i];

    return third_party::SArray<char>(reply_vals);
  }

  virtual void FinishIter() override {}

 private:
  std::map<Key, Val> storage_;
};

}  // namespace csci5570
