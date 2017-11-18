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
    
    LOG(INFO) << results.first << "ms : "<< results.second ;

    auto results2 = Benchmark<std::chrono::nanoseconds>::measure([](std::vector<int> n){
      LOG(INFO) << "In Lambda : " << n.size() ;
    }, vector);
    
    LOG(INFO) << results2 << "ms : ";    
    
}



} // namespace
} // namespace lib
} // namespace csci5570
