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
    AsyncReadBuffer buffer_;    
    buffer_.init("hdfs://proj10:9000/datasets/classification/a9/", 0,1,1024,10240);
}

TEST_F(TestAsyncReadBuffer, DISABLED_get_batch) {
  AsyncReadBuffer buffer_;    
  buffer_.init("hdfs://proj10:9000/datasets/classification/a9/", 0,1,1024,10240);
  
  AsyncReadBuffer::BatchT batch ; 
  while ( !buffer_.end_of_file() || buffer_.ask() > 0) { // only stop when end of file reached and buffer empty.
    if ( buffer_.get_batch(&batch) ){
      for(auto b : batch) { 
        DLOG(INFO) << b;
      }      
    }
  }
  
}

TEST_F(TestAsyncReadBuffer, DISABLED_ask) {
    AsyncReadBuffer buffer_;
    buffer_.init("hdfs://proj10:9000/datasets/classification/a9/", 0,1,1024,10240);
    sleep(1);
    EXPECT_NE(buffer_.ask(), 0);
}

TEST_F(TestAsyncReadBuffer, DISABLED_end_of_file) {
  AsyncReadBuffer buffer_;    
  buffer_.init("hdfs://proj10:9000/datasets/classification/a9/", 0,1,1024,10240);
  while(buffer_.end_of_file() != true){
    DLOG(INFO) << "Waiting end_of_file";
    sleep(1);
  }
}

} // namespace
} // namespace lib
} // namespace csci5570