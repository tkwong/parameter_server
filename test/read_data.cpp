#include <thread>
#include <iostream>
#include <math.h>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "lib/labeled_sample.hpp"
#include "lib/abstract_data_loader.hpp"
#include "driver/engine.cpp"

using namespace csci5570;

typedef lib::LabeledSample<double, int> Sample;
typedef std::vector<Sample> DataStore;
typedef std::function<Sample(boost::string_ref, int)> Parse;

int main(int argc, char** argv)
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_stderrthreshold = 0;
    FLAGS_colorlogtostderr = true;

    std::thread master_thread([]
    {
        HDFSBlockAssigner assigner("proj10", 9000, new zmq::context_t(1), 19818);
        assigner.Serve();
    });

    std::vector<Node> nodes{
        {0, "localhost", 12353}, 
        {1, "localhost", 12354}, 
        {2, "localhost", 12355}
    };

    std::vector<std::thread> threads(nodes.size());
    for (int i = 0; i < nodes.size(); i++)
    {
        DataStore samples;
        lib::AbstractDataLoader<Sample, DataStore>::load<Parse>(
            "hdfs://proj10:9000/datasets/classification/a9/", 123, 
            lib::Parser<Sample, DataStore>::parse_libsvm, &samples, i, nodes.size()
        );

        std::cout << "Got " << samples.size() << " samples" << std::endl;
    }

    master_thread.join();
    return 0;
}
