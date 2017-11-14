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

    std::vector<std::thread*> threads;
    for (int i = 0; i < nodes.size(); i++)
    {
	threads.push_back(new std::thread([=]()
        {
            DataStore samples;
            lib::AbstractDataLoader<Sample, DataStore>::load<Parse>(
                "hdfs://proj10:9000/datasets/classification/avazu-app-part/", 1000000, 
                lib::Parser<Sample, DataStore>::parse_libsvm, &samples, 0, nodes.size(),
                nodes[i].hostname + ':'  + std::to_string(nodes[i].port)
            );

            LOG(INFO) << "Worker " << i << " Got " << samples.size() << " samples" << std::endl;
	}));
    }

    for(auto t : threads) t->join();
    LOG(INFO) << "Loading threads stopped"; 
    master_thread.join();
    LOG(INFO) << "Master thread stopped";
    return 0;
}
