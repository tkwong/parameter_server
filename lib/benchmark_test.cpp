#include <numeric>
#include "glog/logging.h"
#include "gtest/gtest.h"
#include "boost/utility/string_ref.hpp"

#include "lib/benchmark.hpp"


namespace csci5570 {
namespace lib {
namespace {

class TestBenchmark : public testing::Test {
    public:
        TestBenchmark() {}
        ~TestBenchmark() {}

    protected:
        void SetUp() {}
        void TearDown() {}
};

TEST_F(TestBenchmark, measure_execution) {
    Benchmark<> benchmark;

    // auto results = benchmark.benchmark(std::accumulate< Iterator, int>, vector.begin(), vector.end(), 0);
    std::vector<int> vector ({1,2,3});
    // auto result1 = benchmark.benchmark([](std::vector<int> n){
    //   LOG(INFO) << "In Lambda : " << n.size() ;
    //   return n.size();
    // }, vector);

    // std::cout << "#elements: " << std::setw(9) << numEl << " "
    //                   << " mean: " << std::setw(8) << benchmark.mean()
    //                   << " st. dev: : " << std::setw(8)
    //                   << benchmark.standard_deviation()
    //                   << std::endl;
    // auto fn = [](int n, int n2){
    //   LOG(INFO) << "Measure this Execution Time";
    // };
    
    auto results = Benchmark<std::chrono::nanoseconds>::measure_with_result([](std::vector<int> n){
      LOG(INFO) << "In Lambda : " << n.size() ;
      return n.size();
    }, vector);
    
    LOG(INFO) << results.first << "us : "<< results.second ;

    auto results2 = Benchmark<std::chrono::nanoseconds>::measure([](std::vector<int> n){
      LOG(INFO) << "In Lambda : " << n.size() ;
    }, vector);
    
    LOG(INFO) << results2 << "us : ";    
    
}

TEST_F(TestBenchmark, measure_r) {
    Benchmark<> benchmark;

    // auto results = benchmark.benchmark(std::accumulate< Iterator, int>, vector.begin(), vector.end(), 0);
    // std::vector<int> vector ({1,2,3});
    // auto result1 = benchmark.benchmark([](std::vector<int> n){
    //   LOG(INFO) << "In Lambda : " << n.size() ;
    //   return n.size();
    // }, vector);

    // std::cout << "#elements: " << std::setw(9) << numEl << " "
    //                   << " mean: " << std::setw(8) << benchmark.mean()
    //                   << " st. dev: : " << std::setw(8)
    //                   << benchmark.standard_deviation()
    //                   << std::endl;
    // auto fn = [](int n, int n2){
    //   LOG(INFO) << "Measure this Execution Time";
    // };

    auto results1 = benchmark.measure_r([](int n){
      LOG(INFO) << "In Lambda : " << n ;
    }, 0);

    LOG(INFO) << results1 << "us : mean : " << benchmark.mean() << "us std: " << benchmark.standard_deviation();
    
    for (int i = 0 ; i < 10 ; i ++)
    {
      benchmark.measure_r([](int n){LOG(INFO) << "In Lambda : " << n ;}, i);
      LOG(INFO) << results1 << "us : mean : " << benchmark.mean() << "us std: " << benchmark.standard_deviation();
    }
}

TEST_F(TestBenchmark, start_stop) {
    Benchmark<> benchmark;

    // auto results = benchmark.benchmark(std::accumulate< Iterator, int>, vector.begin(), vector.end(), 0);
    std::vector<int> vector ({1,2,3});
    // auto result1 = benchmark.benchmark([](std::vector<int> n){
    //   LOG(INFO) << "In Lambda : " << n.size() ;
    //   return n.size();
    // }, vector);

    // std::cout << "#elements: " << std::setw(9) << numEl << " "
    //                   << " mean: " << std::setw(8) << benchmark.mean()
    //                   << " st. dev: : " << std::setw(8)
    //                   << benchmark.standard_deviation()
    //                   << std::endl;
    // auto fn = [](int n, int n2){
    //   LOG(INFO) << "Measure this Execution Time";
    // };


    for (int i = 0 ; i < 10 ; i ++)
    {
      benchmark.start_measure();      
      sleep(10*rand()/RAND_MAX);
      auto result1 = benchmark.stop_measure();      
      LOG(INFO) << result1 << "us : mean : " << benchmark.mean() << "us std: " << benchmark.standard_deviation();      
    }
}

TEST_F(TestBenchmark, last_n) {
    Benchmark<> benchmark;

    // auto results = benchmark.benchmark(std::accumulate< Iterator, int>, vector.begin(), vector.end(), 0);
    // auto result1 = benchmark.benchmark([](std::vector<int> n){
    //   LOG(INFO) << "In Lambda : " << n.size() ;
    //   return n.size();
    // }, vector);

    // std::cout << "#elements: " << std::setw(9) << numEl << " "
    //                   << " mean: " << std::setw(8) << benchmark.mean()
    //                   << " st. dev: : " << std::setw(8)
    //                   << benchmark.standard_deviation()
    //                   << std::endl;
    // auto fn = [](int n, int n2){
    //   LOG(INFO) << "Measure this Execution Time";
    // };


    for (int i = 0 ; i < 10 ; i ++)
    {
      benchmark.start_measure();      
      sleep(i);
      auto result1 = benchmark.stop_measure();      
      LOG(INFO) << result1 << "us : mean : " << benchmark.mean() << "us std: " << benchmark.standard_deviation();      
    }
    LOG(INFO) << benchmark.last();
    LOG(INFO) << benchmark.last(1);
    LOG(INFO) << benchmark.last(5);
    LOG(INFO) << benchmark.last(10);
    LOG(INFO) << benchmark.last(100);
    
    EXPECT_GE(benchmark.last(), 9000000);
    EXPECT_GE(benchmark.last(1), 9000000);
    EXPECT_GE(benchmark.last(5), 35000000);
    EXPECT_GE(benchmark.last(10), 45000000);
    EXPECT_GE(benchmark.last(100), 45000000);

}

TEST_F(TestBenchmark, pause_resume) {
    Benchmark<> benchmark;

    // auto results = benchmark.benchmark(std::accumulate< Iterator, int>, vector.begin(), vector.end(), 0);
    // auto result1 = benchmark.benchmark([](std::vector<int> n){
    //   LOG(INFO) << "In Lambda : " << n.size() ;
    //   return n.size();
    // }, vector);

    // std::cout << "#elements: " << std::setw(9) << numEl << " "
    //                   << " mean: " << std::setw(8) << benchmark.mean()
    //                   << " st. dev: : " << std::setw(8)
    //                   << benchmark.standard_deviation()
    //                   << std::endl;
    // auto fn = [](int n, int n2){
    //   LOG(INFO) << "Measure this Execution Time";
    // };


    for (int i = 0 ; i < 3 ; i ++)
    {
      benchmark.start_measure();      
      sleep(2);
      benchmark.pause_measure();      
      sleep(2);
      benchmark.resume_measure();      
      sleep(3);
      auto result1 = benchmark.stop_measure();
      LOG(INFO) << benchmark.last() << "us : mean : " << benchmark.mean() << "us std: " << benchmark.standard_deviation();  
      EXPECT_GE(benchmark.last(), 5000000);
      EXPECT_GE(result1, 5000000);
    }

}



} // namespace
} // namespace lib
} // namespace csci5570
