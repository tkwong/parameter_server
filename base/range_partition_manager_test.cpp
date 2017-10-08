#include "glog/logging.h"
#include "gtest/gtest.h"

#include "base/magic.hpp"
#include "base/range_partition_manager.cpp"

namespace csci5570 {
namespace {

class TestRangePartitionManager : public testing::Test {
    public:
        TestRangePartitionManager() {}
        ~TestRangePartitionManager() {}
    
    protected:
        void SetUp() {}
        void TearDown() {}
};

TEST_F(TestRangePartitionManager, Construct) {
    std::vector<uint32_t> vec({0,1});
    RangePartitionManager pm(vec);
}

TEST_F(TestRangePartitionManager, GetNumServers) {
    std::vector<uint32_t> vec({0,1});
    RangePartitionManager pm(vec);
    
    EXPECT_EQ(pm.GetNumServers(), 2);
}

TEST_F(TestRangePartitionManager, GetServerThreadIds) {
    std::vector<uint32_t> vec({0,1});
    RangePartitionManager pm(vec);
    
    EXPECT_EQ(pm.GetServerThreadIds(), vec);
}

TEST_F(TestRangePartitionManager, SliceKeys) {
    using Keys = third_party::SArray<Key>;

    std::vector<uint32_t> in_ids({0,1});
    RangePartitionManager pm(in_ids);
    
    Keys in_keys({1,2,3,4,5});
    std::vector<std::pair<int, Keys>> sliced;
    
    pm.Slice(in_keys, &sliced);
    ASSERT_FALSE(sliced.empty());
    
    std::vector<uint32_t> out_ids;
    Keys out_keys;
    
    for (auto it=sliced.begin(); it!=sliced.end(); it++)
    {
        out_ids.push_back(it->first);
        for (auto it2=(it->second).begin(); it2!=(it->second).end(); it2++)
            out_keys.push_back(*it2);
    }
    
    std::sort(in_ids.begin(), in_ids.end());
    std::sort(out_ids.begin(), out_ids.end());
    std::sort(in_keys.begin(), in_keys.end());
    std::sort(out_keys.begin(), out_keys.end());
    
    EXPECT_EQ(in_ids, out_ids);
    ASSERT_EQ(in_keys.size(), out_keys.size());
    for (int i=0; i<in_keys.size(); i++)
        EXPECT_EQ(in_keys[i], out_keys[i]);
}

TEST_F(TestRangePartitionManager, SliceKeyValuePairs) {
    using KVPairs = std::pair<third_party::SArray<Key>, third_party::SArray<double>>;
    
    std::vector<uint32_t> in_ids({0,1});
    RangePartitionManager pm(in_ids);
    
    KVPairs in_kv({1,2,3,4,5},{1.1,2.2,3.3,4.4,5.5});
    std::vector<std::pair<int, KVPairs>> sliced;
    
    pm.Slice(in_kv, &sliced);
    ASSERT_FALSE(sliced.empty());
    
    std::vector<uint32_t> out_ids;
    third_party::SArray<Key> out_keys;
    third_party::SArray<double> out_values;
    
    for (auto it=sliced.begin(); it!=sliced.end(); it++)
    {
        out_ids.push_back(it->first);
        for (auto it2=(it->second).first.begin(); it2!=(it->second).first.end(); it2++)
            out_keys.push_back(*it2);
        for (auto it2=(it->second).second.begin(); it2!=(it->second).second.end(); it2++)
            out_values.push_back(*it2);
    }
    
    std::sort(in_ids.begin(), in_ids.end());
    std::sort(in_kv.first.begin(), in_kv.first.end());
    std::sort(in_kv.second.begin(), in_kv.second.end());
    std::sort(out_ids.begin(), out_ids.end());
    std::sort(out_keys.begin(), out_keys.end());
    std::sort(out_values.begin(), out_values.end());
    
    EXPECT_EQ(in_ids, out_ids);
    ASSERT_EQ(in_kv.first.size(), out_keys.size());
    ASSERT_EQ(in_kv.second.size(), out_values.size());
    
    for (int i=0; i<out_keys.size(); i++)
    {
        EXPECT_EQ(in_kv.first[i], out_keys[i]);
        EXPECT_EQ(in_kv.second[i], out_values[i]);
    }
}

TEST_F(TestRangePartitionManager, SliceKeysAndKeyValuePairs) {
    using Keys = third_party::SArray<Key>;
    using KVPairs = std::pair<third_party::SArray<Key>, third_party::SArray<double>>;
    
    std::vector<uint32_t> in_ids({0,1});
    RangePartitionManager pm(in_ids);
    
    Keys in_keys({1,2,3,4,5});
    KVPairs in_kv({1,2,3,4,5},{1.1,2.2,3.3,4.4,5.5});
    std::vector<std::pair<int, Keys>> sliced_keys;
    std::vector<std::pair<int, KVPairs>> sliced_kv;
    
    pm.Slice(in_keys, &sliced_keys);
    pm.Slice(in_kv, &sliced_kv);
    ASSERT_FALSE(sliced_keys.empty());
    ASSERT_FALSE(sliced_kv.empty());
    ASSERT_EQ(sliced_keys.size(), sliced_kv.size());
    
    std::map<int, std::vector<Key>> map1, map2;
    for (auto it=sliced_keys.begin(); it!=sliced_keys.end(); it++)
    {
        std::vector<Key> temp((it->second).begin(),(it->second).end());
        map1.insert(std::make_pair(it->first, temp));
    }
    for (auto it=sliced_kv.begin(); it!=sliced_kv.end(); it++)
    {
        std::vector<Key> temp((it->second).first.begin(),(it->second).first.end());
        map2.insert(std::make_pair(it->first, temp));
    }
    
    EXPECT_EQ(map1, map2);
}

} // namespace
} // namespace csci5570
