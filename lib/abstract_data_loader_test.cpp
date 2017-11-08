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

TEST_F(TestAbstractDataLoader, load) {
    std::string hdfs_namenode = "proj10";
    int hdfs_namenode_port = 9000;
    int master_port = 19818;
    zmq::context_t zmq_context(1);

    std::thread master_thread([&zmq_context, master_port, hdfs_namenode_port, hdfs_namenode] {
        HDFSBlockAssigner assigner(hdfs_namenode, hdfs_namenode_port, &zmq_context, master_port);
        assigner.Serve();
    });


    typedef LabeledSample<float, int> Sample;
    LOG(INFO) << "Creating store";
    std::vector<Sample> store;
    LOG(INFO) << "Store created";
    AbstractDataLoader<Sample, std::vector<Sample>>
        ::load<std::function<Sample(boost::string_ref, int)>>(
        "hdfs:///datasets/classification/a9", 14, 
        Parser<Sample, std::vector<Sample>>::parse_libsvm, &store);

    EXPECT_EQ(store.size(), 32561);

    master_thread.join();
}

} // namespace lib
} // namespace csci5570
