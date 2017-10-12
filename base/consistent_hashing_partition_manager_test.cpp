#include "glog/logging.h"
#include "gtest/gtest.h"

#include "base/magic.hpp"
#include "base/consistent_hashing_partition_manager.cpp"
#include "base/abstract_partition_manager.hpp"

namespace csci5570 {
namespace {

class TestConsistentHashingPartitionManager : public testing::Test {
    public:
        TestConsistentHashingPartitionManager() {}
        ~TestConsistentHashingPartitionManager() {}

    protected:
        void SetUp() {
          vec = new std::vector<uint32_t>({0,1,2});
          pm = new ConsistentHashingPartitionManager(*vec);
        }
        void TearDown() {
          delete pm;
          delete vec;
        }
        ConsistentHashingPartitionManager* pm;
        std::vector<uint32_t>* vec;
};

// TEST_F(TestConsistentHashingPartitionManager, Construct) {
//     std::vector<uint32_t> vec({0,1});
//     ConsistentHashingPartitionManager pm(vec);
// }

TEST_F(TestConsistentHashingPartitionManager, GetNumServers) {
    EXPECT_EQ(pm->GetNumServers(), vec->size());
}

TEST_F(TestConsistentHashingPartitionManager, GetServerThreadIds) {
    EXPECT_EQ(pm->GetServerThreadIds(), *vec);
}

TEST_F(TestConsistentHashingPartitionManager, SliceKeys) {

    using Keys = AbstractPartitionManager::Keys;
    Keys in_keys({2,8,9});
    std::vector<std::pair<int, Keys>> sliced;

    // TEST with keys 1,2,3,4,5 and not empty
    pm->Slice(in_keys, &sliced);
    ASSERT_FALSE(sliced.empty());
	
    // ASSERT_EQ(sliced.size(), 3);            // 2 slices for 2 servers
    EXPECT_EQ(sliced[0].first, 0);          // key to server 0
    ASSERT_EQ(sliced[0].second.size(), 2);  // key 2, 8
    EXPECT_EQ(sliced[0].second[0], 2);
    EXPECT_EQ(sliced[0].second[1], 8);
	
    EXPECT_EQ(sliced[1].first, 1);          // keys to server 1
    ASSERT_EQ(sliced[1].second.size(), 1);  // keys 9
    EXPECT_EQ(sliced[1].second[0], 9);

}

TEST_F(TestConsistentHashingPartitionManager, SliceKVs) {
  ConsistentHashingPartitionManager pm({0, 1, 2});
  third_party::SArray<Key> keys({2, 5, 9});
  third_party::SArray<double> vals({.2, .5, .9});
  std::vector<std::pair<int, AbstractPartitionManager::KVPairs>> sliced;
  pm.Slice(std::make_pair(keys, vals), &sliced);

  // Expected Results : 
  /*
  I1013 00:04:39.438887 3080803264 consistent_hashing_partition_manager.cpp:71] Key: 2, Val:0.2, Node ID:0 Hash:(9962920844341270757)
  I1013 00:04:39.438916 3080803264 consistent_hashing_partition_manager.cpp:71] Key: 5, Val:0.5, Node ID:1 Hash:(9840292967791829051)
  I1013 00:04:39.438925 3080803264 consistent_hashing_partition_manager.cpp:71] Key: 9, Val:0.9, Node ID:1 Hash:(4245867132641434420)
  */


  ASSERT_EQ(sliced.size(), 2);  // 2 slices for 2 servers
  EXPECT_EQ(sliced[0].first, 0);
  EXPECT_EQ(sliced[1].first, 1);
  // EXPECT_EQ(sliced[2].first, 2);
  ASSERT_EQ(sliced[0].second.first.size(), 1);  // key 2
  EXPECT_EQ(sliced[0].second.first[0], 2);
  ASSERT_EQ(sliced[0].second.second.size(), 1);  // value .2
  EXPECT_DOUBLE_EQ(sliced[0].second.second[0], .2);
  ASSERT_EQ(sliced[1].second.first.size(), 2);  // key 5, 9
  EXPECT_EQ(sliced[1].second.first[0], 5);
  EXPECT_EQ(sliced[1].second.first[1], 9);
  ASSERT_EQ(sliced[1].second.second.size(), 2);  // value .5, .9
  EXPECT_DOUBLE_EQ(sliced[1].second.second[0], .5);
  EXPECT_DOUBLE_EQ(sliced[1].second.second[1], .9);
  // ASSERT_EQ(sliced[2].second.first.size(), 1);  // key 9
  // EXPECT_EQ(sliced[2].second.first[0], 9);
  // ASSERT_EQ(sliced[2].second.second.size(), 1);  // value .9
  // EXPECT_DOUBLE_EQ(sliced[2].second.second[0], .9);
}

// TEST_F(TestConsistentHashingPartitionManager, SliceKeysIfNodeDeleted) {GTEST_FAIL();}
// TEST_F(TestConsistentHashingPartitionManager, SliceKeysIfNodeAdded) {GTEST_FAIL();}
// TEST_F(TestConsistentHashingPartitionManager, SliceKeyValuePairs) {GTEST_FAIL();}
// TEST_F(TestConsistentHashingPartitionManager, SliceKeyValuePairsIfNodeDeleted) {GTEST_FAIL();}
// TEST_F(TestConsistentHashingPartitionManager, SliceKeyValuePairsIfNodeAdded) {GTEST_FAIL();}

} // namespace
} // namespace csci5570
