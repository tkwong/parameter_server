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

TEST_F(TestAsyncReadBuffer, init) {
  
  std::thread master_thread([]{
      HDFSBlockAssigner assigner("localhost", 9000, new zmq::context_t(1), 19818);
      assigner.Serve();
  });
  
  AsyncReadBuffer buffer_;    
  buffer_.init("hdfs://proj10:9000/datasets/classification/a9/",0,1,"localhost", 19818, "localhost", 1024);
  
  master_thread.join();
}

TEST_F(TestAsyncReadBuffer, get_batch) {
  
  std::thread master_thread([]{
      HDFSBlockAssigner assigner("proj10", 9000, new zmq::context_t(1), 19818);
      assigner.Serve();
  });
    
  AsyncReadBuffer buffer_;    
  buffer_.init("hdfs://proj10:9000/datasets/classification/a9/",0,1,"proj10", 19818, "proj10", 10);
  
  int count = 0;
  while ( !buffer_.end_of_file() || buffer_.ask() > 0) { // only stop when end of file reached and buffer empty.
    AsyncReadBuffer::BatchT batch;
    if ( buffer_.get_batch(&batch) ){
      EXPECT_EQ(batch.size(), 10);
      count += batch.size();
    }
  }
  
  EXPECT_EQ(count, 32561);
  master_thread.join();
  
}

TEST_F(TestAsyncReadBuffer, queue) {

moodycamel::BlockingConcurrentQueue<std::string> q;
std::thread producer([&]() {
    for (int i = 0; i != 100; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(i % 10));
		std::string record_("TEST\nTEST\nTEST");
		boost::char_separator<char> sep{"\n"};
		boost::tokenizer<boost::char_separator<char> > tok{record_, sep};
		for (const auto &t : tok)
		{
		  q.enqueue("TEST");
		}
        //q.enqueue(i);
    }
});
std::thread consumer([&]() {
    for (int i = 0; i != 100; ++i) {
        std::string item;
        q.wait_dequeue(item);
       	EXPECT_EQ(item ,"TEST");
        
        if (q.wait_dequeue_timed(item, std::chrono::milliseconds(5))) {
            ++i;
            EXPECT_EQ(item ,"TEST");
        }
    }
});
producer.join();
consumer.join();
}

// TEST_F(TestAsyncReadBuffer, ask) {
//     AsyncReadBuffer buffer_;
//     buffer_.init("hdfs://proj10:9000/datasets/classification/a9/", 0,1,1024,10240);
//     sleep(1);
//     EXPECT_NE(buffer_.ask(), 0);
// }
//
// TEST_F(TestAsyncReadBuffer, end_of_file) {
//   AsyncReadBuffer buffer_;
//   buffer_.init("hdfs://proj10:9000/datasets/classification/a9/", 0,1,1024,10240);
//   while(buffer_.end_of_file() != true){
//     DLOG(INFO) << "Waiting end_of_file";
//     sleep(1);
//   }
// }

} // namespace
} // namespace lib
} // namespace csci5570

