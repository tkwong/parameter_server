#include <thread>

#include "glog/logging.h"
#include "gtest/gtest.h"

#include "lib/abstract_data_loader.hpp"
#include "lib/labeled_sample.hpp"
#include "lib/parser.hpp"

namespace csci5570 {
namespace lib {

class TestAbstractDataLoader : public testing::Test {
    public:
        TestAbstractDataLoader() {}
        ~TestAbstractDataLoader() {}
    protected:
        void SetUp() {}
        void TearDown() {}
};

TEST_F(TestAbstractDataLoader, load)
{
    std::thread master_thread([]{
        HDFSBlockAssigner assigner("proj10", 9000, new zmq::context_t(1), 19818);
        assigner.Serve();
    });

    typedef LabeledSample<float, int> Sample;
    LOG(INFO) << "Creating store";
    std::vector<Sample> store;
    LOG(INFO) << "Store created";

    AbstractDataLoader<Sample, std::vector<Sample>>
        ::load<std::function<Sample(boost::string_ref, int)>>(
        "hdfs://proj10:9000/datasets/classification/a9/", 123, 
        Parser<Sample, std::vector<Sample>>::parse_libsvm, &store, 0, 1);

    EXPECT_EQ(store.size(), 32561);
    master_thread.join();
}

} // namespace lib
} // namespace csci5570
