#include "glog/logging.h"
#include "gtest/gtest.h"

#include "worker/callback_runner.cpp"

namespace csci5570 {

class TestCallbackRunner: public testing::Test {
    protected:
        void SetUp() {}
        void TearDown() {}
}; // class TestCallbackRunner

TEST_F(TestCallbackRunner, Init) 
{
    CallbackRunner runner;
}

TEST_F(TestCallbackRunner, AddResponse) 
{
    CallbackRunner runner;
    std::map<Key,float> reply;
    bool finished = false;
    
    runner.RegisterRecvHandle(0, 0,[&reply](Message& msg){
        auto re_keys = third_party::SArray<Key>(msg.data[0]);
        auto re_vals = third_party::SArray<float>(msg.data[1]);

        for (int i=0; i<re_keys.size(); i++) reply.insert(std::make_pair(re_keys[i], re_vals[i]));
    });
    runner.RegisterRecvFinishHandle(0, 0, [&finished]{finished=true;});
    runner.NewRequest(0, 0, 2);

    Message r1, r2;
    third_party::SArray<Key> r1_keys{3};
    third_party::SArray<float> r1_vals{0.1};
    r1.AddData(r1_keys);
    r1.AddData(r1_vals);
    third_party::SArray<Key> r2_keys{4, 5, 6};
    third_party::SArray<float> r2_vals{0.4, 0.2, 0.3};
    r2.AddData(r2_keys);
    r2.AddData(r2_vals);
    runner.AddResponse(0, 0, r1);
    runner.AddResponse(0, 0, r2);
    
    runner.WaitRequest(0, 0);
    
    EXPECT_EQ(reply.size(), 4);
    std::map<Key, float> expected {
    {3, 0.1}, 
    {4, 0.4}, 
    {5, 0.2}, 
    {6, 0.3}
    };
    EXPECT_EQ(reply, expected);
    EXPECT_EQ(finished, true);
}

TEST_F(TestCallbackRunner, AddResponseTwoWorkers) 
{
    CallbackRunner runner;
    std::map<Key,float> reply;
    float sum = 0;
    
    // Register for first worker
    runner.RegisterRecvHandle(0, 0,[&reply](Message& msg){
        auto re_keys = third_party::SArray<Key>(msg.data[0]);
        auto re_vals = third_party::SArray<float>(msg.data[1]);

        for (int i=0; i<re_keys.size(); i++) reply.insert(std::make_pair(re_keys[i], re_vals[i]));
    });
    runner.RegisterRecvFinishHandle(0, 0, []{});
    runner.NewRequest(0, 0, 2);
    
    // Register for second worker
    runner.RegisterRecvHandle(1, 0,[&sum](Message& msg){
        auto re_vals = third_party::SArray<float>(msg.data[1]);
        for (int i=0; i<re_vals.size(); i++) sum += re_vals[i];
    });
    runner.RegisterRecvFinishHandle(1, 0, []{});
    runner.NewRequest(1, 0, 2);

    // Respond to both worker
    Message r1, r2;
    third_party::SArray<Key> r1_keys{3};
    third_party::SArray<float> r1_vals{0.1};
    r1.AddData(r1_keys);
    r1.AddData(r1_vals);
    third_party::SArray<Key> r2_keys{4, 5, 6};
    third_party::SArray<float> r2_vals{0.4, 0.2, 0.3};
    r2.AddData(r2_keys);
    r2.AddData(r2_vals);
    runner.AddResponse(0, 0, r1);
    runner.AddResponse(0, 0, r2);
    runner.AddResponse(1, 0, r1);
    runner.AddResponse(1, 0, r2);
    
    runner.WaitRequest(0, 0);
    EXPECT_EQ(reply.size(), 4);
    std::map<Key, float> expected {
    {3, 0.1}, 
    {4, 0.4}, 
    {5, 0.2}, 
    {6, 0.3}
    };
    EXPECT_EQ(reply, expected);
    
    runner.WaitRequest(1, 0);
    EXPECT_EQ(sum, 1.0);
}

} // namespace csci5570
