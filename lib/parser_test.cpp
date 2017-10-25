#include "glog/logging.h"
#include "gtest/gtest.h"
#include "boost/utility/string_ref.hpp"

#include "lib/parser.hpp"
#include "lib/labeled_sample.hpp"
#include "eigen3/Eigen/Sparse"

namespace csci5570 {
namespace lib {
namespace {

class TestParser : public testing::Test {
    public:
        TestParser() {}
        ~TestParser() {}
    
    protected:
        void SetUp() {}
        void TearDown() {}
};

TEST_F(TestParser, libsvm) {
    boost::string_ref line1 = "-1 35:1 48:1 70:1 149:1 250:1";
    boost::string_ref line2 = "20 99:1.1 207:1.2 208:1.3 225:1.4";
    
    auto sample1 = Parser<LabeledSample<int, int>, 
        Eigen::SparseVector<int>>::parse_libsvm(line1, 5);
        
    Eigen::SparseVector<int> expected1;
    expected1.resize(251);
    expected1.coeffRef(35)  = 1;
    expected1.coeffRef(48)  = 1;
    expected1.coeffRef(70)  = 1;
    expected1.coeffRef(149) = 1;
    expected1.coeffRef(250) = 1;

    EXPECT_EQ(sample1.features.sum(), expected1.sum());
    EXPECT_EQ(sample1.label, -1);
    
    auto sample2 = Parser<LabeledSample<char, double>, 
        Eigen::SparseVector<double>>::parse_libsvm(line2, 4);
    
    Eigen::SparseVector<int> expected2;
    expected2.resize(226);
    expected2.coeffRef(99)  = 1.1;
    expected2.coeffRef(207) = 1.2;
    expected2.coeffRef(208) = 1.3;
    expected2.coeffRef(225) = 1.4;

    EXPECT_EQ(sample2.features.sum(), expected2.sum());
    EXPECT_EQ(sample2.label, 20);
}

} // namespace
} // namespace lib
} // namespace csci5570
