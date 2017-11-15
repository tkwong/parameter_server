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
          // vec = new std::vector<uint32_t>({0,1,2});
          // pm = new ConsistentHashingPartitionManager(*vec);
        }
        void TearDown() {
          // delete pm;
          // delete vec;
        }
        // ConsistentHashingPartitionManager* pm;
        // std::vector<uint32_t>* vec;
};

TEST_F(TestConsistentHashingPartitionManager, Construct) {
    std::vector<uint32_t> vec({0,1});
    ConsistentHashingPartitionManager pm(vec);
}

TEST_F(TestConsistentHashingPartitionManager, GetNumServers) {
	std::vector<uint32_t> vec ({0,1,2});
    ConsistentHashingPartitionManager pm(vec);

    EXPECT_EQ(pm.GetNumServers(), vec.size());
}

TEST_F(TestConsistentHashingPartitionManager, GetServerThreadIds) {
	std::vector<uint32_t> vec ({0,1,2});
	ConsistentHashingPartitionManager pm(vec);

    EXPECT_EQ(pm.GetServerThreadIds(), vec);
}

TEST_F(TestConsistentHashingPartitionManager, SliceKeys) {

    using Keys = AbstractPartitionManager::Keys;

    ConsistentHashingPartitionManager pm({0, 1, 2});
    third_party::SArray<Key> in_keys({2, 8, 9});
	std::vector<std::pair<int, Keys>> sliced;

    // TEST with keys 1,2,3,4,5 and not empty
    pm.Slice(in_keys, &sliced);
    ASSERT_FALSE(sliced.empty());

	/*
		I1016 20:33:45.771512 3080803264 consistent_hashing_partition_manager.cpp:26] Key: 2, Node ID:0
		I1016 20:33:45.772068 3080803264 consistent_hashing_partition_manager.cpp:37] ADD 2 to 0
		I1016 20:33:45.772075 3080803264 consistent_hashing_partition_manager.cpp:26] Key: 8, Node ID:0
		I1016 20:33:45.772083 3080803264 consistent_hashing_partition_manager.cpp:34] APPEND 8 to 0
		I1016 20:33:45.772086 3080803264 consistent_hashing_partition_manager.cpp:26] Key: 9, Node ID:2
		I1016 20:33:45.772092 3080803264 consistent_hashing_partition_manager.cpp:37] ADD 9 to 2
	*/
    // ASSERT_EQ(sliced.size(), 3);            // 2 slices for 2 servers
    EXPECT_EQ(sliced[0].first, 0);          // key to server 0
    ASSERT_EQ(sliced[0].second.size(), 2);  // key 2, 8
    EXPECT_EQ(sliced[0].second[0], 2);
    EXPECT_EQ(sliced[0].second[1], 8);

    EXPECT_EQ(sliced[1].first, 2);          // keys to server 1
    ASSERT_EQ(sliced[1].second.size(), 1);  // keys 9
    EXPECT_EQ(sliced[1].second[0], 9);

    third_party::SArray<Key> keys({2, 8, 9, 10, 11,12,13});
    pm.Slice(keys, &sliced);

	/*
	I1016 20:33:45.772099 3080803264 consistent_hashing_partition_manager.cpp:26] Key: 2, Node ID:0
	I1016 20:33:45.772104 3080803264 consistent_hashing_partition_manager.cpp:37] ADD 2 to 0
	I1016 20:33:45.772109 3080803264 consistent_hashing_partition_manager.cpp:26] Key: 8, Node ID:0
	I1016 20:33:45.772114 3080803264 consistent_hashing_partition_manager.cpp:34] APPEND 8 to 0
	I1016 20:33:45.772120 3080803264 consistent_hashing_partition_manager.cpp:26] Key: 9, Node ID:2
	I1016 20:33:45.772125 3080803264 consistent_hashing_partition_manager.cpp:37] ADD 9 to 2
	I1016 20:33:45.772130 3080803264 consistent_hashing_partition_manager.cpp:26] Key: 10, Node ID:2
	I1016 20:33:45.772135 3080803264 consistent_hashing_partition_manager.cpp:34] APPEND 10 to 2
	I1016 20:33:45.772140 3080803264 consistent_hashing_partition_manager.cpp:26] Key: 11, Node ID:2
	I1016 20:33:45.772145 3080803264 consistent_hashing_partition_manager.cpp:34] APPEND 11 to 2
	I1016 20:33:45.772150 3080803264 consistent_hashing_partition_manager.cpp:26] Key: 12, Node ID:1
	I1016 20:33:45.772156 3080803264 consistent_hashing_partition_manager.cpp:37] ADD 12 to 1
	I1016 20:33:45.772159 3080803264 consistent_hashing_partition_manager.cpp:26] Key: 13, Node ID:0
	I1016 20:33:45.772241 3080803264 consistent_hashing_partition_manager.cpp:34] APPEND 13 to 0
	*/
    ASSERT_EQ(sliced.size(), 3);            // 3 slices for 3 servers
    EXPECT_EQ(sliced[0].first, 0);          // key to server 0
    EXPECT_EQ(sliced[1].first, 2);          // keys to server 2
    ASSERT_EQ(sliced[0].second.size(), 3);  // key 2
    ASSERT_EQ(sliced[1].second.size(), 3);  // keys 12
    EXPECT_EQ(sliced[0].second[0], 2);
    EXPECT_EQ(sliced[1].second[0], 9);
    EXPECT_EQ(sliced[1].second[1], 10);

}


TEST_F(TestConsistentHashingPartitionManager, SliceKVs) {
  ConsistentHashingPartitionManager pm({0, 1, 2});
  third_party::SArray<Key> keys({2, 5, 9});
  third_party::SArray<double> vals({.2, .5, .9});
  std::vector<std::pair<int, AbstractPartitionManager::KVPairs<>>> sliced;
  pm.Slice(std::make_pair(keys, vals), &sliced);

  // Expected Results :
  /*
	I1016 20:33:45.772344 3080803264 consistent_hashing_partition_manager.cpp:54] Key: 2, Node ID:0
	I1016 20:33:45.772356 3080803264 consistent_hashing_partition_manager.cpp:69] ADD 2 to 0
	I1016 20:33:45.772361 3080803264 consistent_hashing_partition_manager.cpp:54] Key: 5, Node ID:1
	I1016 20:33:45.772367 3080803264 consistent_hashing_partition_manager.cpp:69] ADD 5 to 1
	I1016 20:33:45.772372 3080803264 consistent_hashing_partition_manager.cpp:54] Key: 9, Node ID:2
	I1016 20:33:45.772382 3080803264 consistent_hashing_partition_manager.cpp:69] ADD 9 to 2
  */


  ASSERT_EQ(sliced.size(), 3);  // 2 slices for 2 servers
  EXPECT_EQ(sliced[0].first, 0);
  EXPECT_EQ(sliced[1].first, 1);
  // EXPECT_EQ(sliced[2].first, 2);
  ASSERT_EQ(sliced[0].second.first.size(), 1);  // key 2
  EXPECT_EQ(sliced[0].second.first[0], 2);
  ASSERT_EQ(sliced[0].second.second.size(), 1);  // value .2
  EXPECT_DOUBLE_EQ(sliced[0].second.second[0], .2);
  ASSERT_EQ(sliced[1].second.first.size(), 1);  // key 5
  EXPECT_EQ(sliced[1].second.first[0], 5);
  ASSERT_EQ(sliced[1].second.second.size(), 1);  // value .5
  EXPECT_DOUBLE_EQ(sliced[1].second.second[0], .5);
}

// TEST_F(TestConsistentHashingPartitionManager, SliceKeysIfNodeDeleted) {GTEST_FAIL();}
// TEST_F(TestConsistentHashingPartitionManager, SliceKeysIfNodeAdded) {GTEST_FAIL();}
// TEST_F(TestConsistentHashingPartitionManager, SliceKeyValuePairs) {GTEST_FAIL();}
// TEST_F(TestConsistentHashingPartitionManager, SliceKeyValuePairsIfNodeDeleted) {GTEST_FAIL();}
// TEST_F(TestConsistentHashingPartitionManager, SliceKeyValuePairsIfNodeAdded) {GTEST_FAIL();}

} // namespace
} // namespace csci5570
