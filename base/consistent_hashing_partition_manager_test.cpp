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

TEST_F(TestConsistentHashingPartitionManager, SliceKeysIfNodeDeleted) {GTEST_FAIL();}
TEST_F(TestConsistentHashingPartitionManager, SliceKeysIfNodeAdded) {GTEST_FAIL();}
TEST_F(TestConsistentHashingPartitionManager, SliceKeyValuePairs) {GTEST_FAIL();}
TEST_F(TestConsistentHashingPartitionManager, SliceKeyValuePairsIfNodeDeleted) {GTEST_FAIL();}
TEST_F(TestConsistentHashingPartitionManager, SliceKeyValuePairsIfNodeAdded) {GTEST_FAIL();}

} // namespace
} // namespace csci5570
