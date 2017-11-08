#include "glog/logging.h"
#include "gtest/gtest.h"
#include "boost/utility/string_ref.hpp"

#include "lib/abstract_async_data_loader.hpp"

namespace csci5570 {
namespace lib {
namespace {

class TestAsyncDataLoader : public testing::Test {
    public:
        TestAsyncDataLoader() {}
        ~TestAsyncDataLoader() {}
    
    protected:
        void SetUp() {}
        void TearDown() {}
};

TEST_F(TestAsyncDataLoader, init) {
    
}

} // namespace
} // namespace lib
} // namespace csci5570
