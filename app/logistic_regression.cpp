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

    std::vector<Node> nodes{{0, "localhost", 12353}, {1, "localhost", 12354}};

    std::vector<std::thread*> threads(nodes.size());
    for (int i = 0; i < nodes.size(); i++)
    {
        DataStore samples;
        lib::AbstractDataLoader<Sample, DataStore>::load<Parse>(
            "hdfs://proj10:9000/datasets/classification/a9/", 123, 
            lib::Parser<Sample, DataStore>::parse_libsvm, &samples, i, nodes.size()
        );

        DataStore node_samples(samples.begin() + 15000 * i, samples.begin() + 15000 * (i + 1));
        DataStore test_samples(samples.begin() + 30000, samples.end());

        threads[i] = new std::thread([&nodes, i, node_samples, test_samples]()
        {
            std::cout << "Got " << node_samples.size() << " samples" << std::endl;

            Engine engine(nodes[i], nodes);
            engine.StartEverything();

            auto table_id = engine.CreateTable<double>(ModelType::BSP, StorageType::Map, 3);
            engine.Barrier();

            MLTask task;
            task.SetWorkerAlloc({{0, 3}, {1, 3}});
            task.SetTables({table_id});
            task.SetLambda([table_id, node_samples, test_samples](const Info& info)
            {
                std::cout << "Worker id: " << info.worker_id << " table id: " << table_id
                          << std::endl;

                KVClientTable<double> table = info.CreateKVClientTable<double>(table_id);

                DataStore thread_samples(node_samples.begin() + 5000 * (info.worker_id % 3), 
                                         node_samples.begin() + 5000 * (info.worker_id % 3 + 1));

                std::cout << "Worker " << info.worker_id << " got " << thread_samples.size()
                          << " samples." << std::endl;

                //std::cout << "Initializing keys" << std::endl;
                // Initialize all keys with 0
                std::vector<Key> keys;
                for (int i=0; i<124; i++) keys.push_back(i);
                std::vector<double> init_vals(124);
                table.Add(keys, init_vals);
                //std::cout << "Key Initialized" << std::endl;

                // Train
                int i = 0;
                double learning_rate = 0.001;
                for (Sample sample : thread_samples)
                {
                    // Pull
                    std::vector<double> vals;
                    table.Get(keys, &vals);
                    //std::cout << "Got vals: ";
                    //for (auto val : vals) std::cout << val << ' ';
                    //std::cout << std::endl;

                    // Predict
                    double yhat = vals.at(0);
                    for (Eigen::SparseVector<double>::InnerIterator it(sample.features); it; ++it)
                        yhat += vals.at(it.index()) * it.value();
                    double predict = 1.0 / (1.0 + exp(-yhat));

                    //std::cout << "Predict: " << predict << std::endl;

                    // Cal Error and Gradient
                    double error = (sample.label > 0 ? 1 : 0) - predict;
                    double gradient = vals.at(0);
                    for (Eigen::SparseVector<double>::InnerIterator it(sample.features); it; ++it)
                        gradient += it.value() * error;

                    //std::cout << "Error: " << error << " Gradient: " << gradient << std::endl;

                    // Update
                    vals.at(0) += learning_rate * gradient;
                    for (Eigen::SparseVector<double>::InnerIterator it(sample.features); it; ++it)
                        vals.at(it.index()) += learning_rate * gradient;

                    // Push
                    //std::cout << "Adding vals: ";
                    //for (auto val : vals) std::cout << val << ' ';
                    //std::cout << std::endl;
                    table.Add(keys, vals);
                    table.Clock();

                    //if (++i > 0) break;
                }

                // Test
                std::vector<double> vals;
                table.Get(keys, &vals);

                std::cout << "Printing out first 10 coeff:" << std::endl;
                for (int i=0; i<10; i++) std::cout << vals.at(i) << ' ';
                std::cout << std::endl;

                int correct = 0;
                for (Sample sample : test_samples)
                {
                    double yhat = vals.at(0);
                    for (Eigen::SparseVector<double>::InnerIterator it(sample.features); it; ++it)
                        yhat += vals.at(it.index()) * it.value();
                    double estimate = 1.0 / (1.0 + exp(-yhat));

                    int est_class = (estimate > 0.5 ? 1 : 0);
                    int answer = (sample.label > 0 ? 1 : 0);
                    if (est_class == answer) correct++;
                }
                std::cout << "Accuracy: " << correct << " out of " << test_samples.size() 
                          << " " << float(correct)/test_samples.size()*100 << " percent" 
                          << std::endl;
            });

            engine.Run(task);
            engine.StopEverything();
        });
    }

    for (auto thread : threads) thread->join();
    master_thread.join();
    return 0;
}
