#include "glog/logging.h"
#include "gtest/gtest.h"
#include "boost/utility/string_ref.hpp"

#include "lib/parser.hpp"
#include "lib/labeled_sample.hpp"
#include "Eigen/Sparse"

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
	GTEST_FAIL();
    boost::string_ref line1 = "-1 35:1 48:1 70:1 149:1 250:1";
    boost::string_ref line2 = "+1 99:1 207:1 208:1 225:1";
    
    LabeledSample<Eigen::SparseVector<int>, int> sample1;
    sample1 = Parser<LabeledSample<Eigen::SparseVector<int>, int>, Eigen::SparseVector<int>>::
        parse_libsvm(line1, 5);
        
    Eigen::SparseVector<int> expected;
    expected.coeffRef(35)=1;
    expected.coeffRef(48)=1;
    expected.coeffRef(70)=1;
    expected.coeffRef(149)=1;
    expected.coeffRef(250)=1;

    EXPECT_EQ(sample1.x_.sum(), expected.sum());
    EXPECT_EQ(sample1.y_, -1);
}

} // namespace
} // namespace lib
} // namespace csci5570
