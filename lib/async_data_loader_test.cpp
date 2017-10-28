#include "glog/logging.h"
#include "gtest/gtest.h"
#include "boost/utility/string_ref.hpp"

#include "lib/abstract_async_data_loader.hpp"

namespace csci5570 {
namespace lib {
namespace {

class TestAsyncReadBuffer : public testing::Test {
    public:
        TestAsyncReadBuffer() {}
        ~TestAsyncReadBuffer() {}
    
    protected:
        void SetUp() {}
        void TearDown() {}
};

TEST_F(TestAsyncReadBuffer, DISABLED_init) {
    
}

TEST_F(TestAsyncReadBuffer, DISABLED_get_batch) {
    
}

TEST_F(TestAsyncReadBuffer, ask) {
    // AsyncReadBuffer buffer_ ();
    // buffer_.init("url", 0,1,1,1);
    // EXPECT_EQ(buffer_.ask(), 1);
}

TEST_F(TestAsyncReadBuffer, end_of_file) {
    
}

} // namespace
} // namespace lib
} // namespace csci5570
