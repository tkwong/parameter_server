#include "glog/logging.h"
#include "gtest/gtest.h"

#include "server/simple_server.cpp"

namespace csci5570 {
namespace {

class TestSimpleServer : public testing::Test {
    public:
        TestSimpleServer() {}
        ~TestSimpleServer() {}
    
    protected:
        void SetUp() {}
        void TearDown() {}
};

TEST_F(TestSimpleServer, Construct) {
    SimpleServer(2);
}

} // namespace
} // namespace csci5570
