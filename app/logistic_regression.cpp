#include <thread>
#include <iostream>
#include <math.h>
#include <numeric>
#include <algorithm>
#include <chrono>
#include <cmath>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "lib/labeled_sample.hpp"
#include "lib/abstract_data_loader.hpp"
#include "driver/engine.cpp"
#include <fstream>
#include "lib/benchmark.hpp"



using namespace csci5570;

typedef lib::LabeledSample<double, int> Sample;
typedef std::vector<Sample> DataStore;
typedef std::function<Sample(boost::string_ref, int)> Parse;

// System info
DEFINE_int32(my_id, 0, "The host ID of this program");
DEFINE_string(config_file, "", "The config file path");
// Data loading config
// DEFINE_string(hdfs_namenode, "localhost", "The hdfs namenode hostname");
// DEFINE_int32(hdfs_namenode_port, 9000, "The hdfs namenode port");
// DEFINE_string(hdfs_master_host, "localhost", "A hostname for the hdfs assigner host");
DEFINE_int32(hdfs_master_node_id, 0, "The Node ID of the HDFS Assigner");
DEFINE_int32(hdfs_master_port, 19818, "A port number for the hdfs assigner host");
DEFINE_int32(n_loaders_per_node, 1, "The number of loaders per node");
DEFINE_string(input, "", "The hdfs input url");
DEFINE_int32(n_features, -1, "The number of features in the dataset");
// Training config
DEFINE_int32(n_workers_per_node, 1, "The number of loaders per node");
DEFINE_int32(n_iters, 10, "The number of iterations");
DEFINE_int32(batch_size, 100, "Batch size");
DEFINE_double(alpha, 0.0001, "learning rate");

// DEFINE_validator(my_id, [](const char* flagname, int value){ return value>=0;});
// DEFINE_validator(config_file, [](const char* flagname, std::string value){ return false;});
DEFINE_validator(input, [](const char* flagname, const std::string& value){ return !value.empty();});
DEFINE_validator(n_features, [](const char* flagname, int value){ return value>=0;});

int main(int argc, char** argv)
{
    gflags::SetUsageMessage("Usage : LogisticRegression ");
    gflags::SetVersionString("1.0.0");
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    FLAGS_stderrthreshold = 0;
    FLAGS_colorlogtostderr = true;
    
        
    // Parse config_file
    std::vector<Node> nodes;
    
    if(FLAGS_config_file.empty()){
      Node node;
      node.id = 0;
      node.hostname = "localhost";
      node.port = 62353;
      LOG(INFO) << "Loaded Default Nodes :" << node.DebugString() ;
      nodes.push_back(std::move(node));
    } else {
      // std::vector<Node> nodes{{0, "localhost", 12353}, {1, "localhost", 12354},
      //                         {2, "localhost", 12355}, {3, "localhost", 12356},
      //                         {4, "localhost", 12357}};
    
      std::ifstream input_file(FLAGS_config_file.c_str());
      CHECK(input_file.is_open()) << "Error opening file: " << FLAGS_config_file;
      std::string line;
      while (getline(input_file, line)) {
        size_t id_pos = line.find(":");
        CHECK_NE(id_pos, std::string::npos);
        std::string id = line.substr(0, id_pos);
        size_t host_pos = line.find(":", id_pos+1);
        CHECK_NE(host_pos, std::string::npos);
        std::string hostname = line.substr(id_pos+1, host_pos - id_pos - 1);
        std::string port = line.substr(host_pos+1, line.size() - host_pos - 1);
      
        try {
          Node node;
          node.id = std::stoi(id);
          node.hostname = std::move(hostname);
          node.port = std::stoi(port);
          LOG(INFO) << "Loaded Node from config: " << node.DebugString() ; 
          nodes.push_back(std::move(node));
        }
        catch(const std::invalid_argument& ia) {
          LOG(FATAL) << "Invalid argument: " << ia.what() << "\n";
        }
      }
      LOG(INFO) << "Loaded Node from config file";
        }
    // 
    const Node* my_node;
    for (const auto& node : nodes) {
      if (FLAGS_my_id == node.id) {
        my_node = &node;
      }
    }
    
    LOG(INFO) << "MyNode : "<< my_node->DebugString();

    // if it is HDFSBlockAssigner host, start the thread
    if(FLAGS_hdfs_master_node_id == my_node->id)
    {
      LOG(INFO) << "Starting HDFS Assigner for ["<< my_node->id << "] " << my_node->hostname << ":" << FLAGS_hdfs_master_port;
      std::thread master_thread([]()
      {
          LOG(INFO) << "Using HDFS input " << FLAGS_input ;
          // Parse from input
          LUrlParser::clParseURL URL = LUrlParser::clParseURL::ParseURL(FLAGS_input);
          std::string hdfs_namenode = URL.m_Host;
          int hdfs_namenode_port = stoi(URL.m_Port);
          
          LOG(INFO) << "Using HDFS " << hdfs_namenode << ":" << hdfs_namenode_port;
        
          HDFSBlockAssigner assigner(hdfs_namenode, hdfs_namenode_port, new zmq::context_t(1), FLAGS_hdfs_master_port);
          assigner.Serve();
      });
      master_thread.detach();
    }
    
    // Load Data Samples
    DataStore samples;
    std::string hdfs_master_host(""); 
    for (const auto& node : nodes) {
      if (FLAGS_hdfs_master_node_id == node.id) {
        LOG(INFO) << "Using HDFS Assigner from "<< node.id << " : " << node.hostname;
        hdfs_master_host = node.hostname;
      }
    }
    CHECK(!hdfs_master_host.empty()) << "No HDFS Assigner Host defined";
      
    lib::AbstractDataLoader<Sample, DataStore>::load<Parse>(
        FLAGS_input, FLAGS_n_features, 
        lib::Parser<Sample, DataStore>::parse_libsvm, &samples, 0, nodes.size(), hdfs_master_host, FLAGS_hdfs_master_port, my_node->hostname
    );

    DataStore node_samples(samples.begin() , samples.end() - 10000 );
    DataStore test_samples(samples.end() - 10000, samples.end());
        
    LOG(INFO) << "Got " << node_samples.size() << " samples";

    Engine* engine = new Engine(*my_node, nodes);
    engine->StartEverything();

    auto table_id = engine->CreateTable<double>(ModelType::BSP, StorageType::Map, 3);
    engine->Barrier();

    MLTask task;
    std::vector<WorkerAlloc> worker_alloc;
    for (auto& node : nodes) {
      worker_alloc.push_back({node.id, static_cast<uint32_t>(FLAGS_n_workers_per_node)}); 
    }
    // task.SetWorkerAlloc({{0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3}});
    task.SetWorkerAlloc(worker_alloc);
    task.SetTables({table_id});
    
    task.SetLambda([table_id, node_samples, test_samples](const Info& info)
    {
#ifdef BENCHMARK
        auto benchmark = Benchmark<>::measure([&](const Info& info) {
#endif
        
        LOG(INFO) << "Worker id: " << info.worker_id << " table id: " << table_id;

        KVClientTable<double> table = info.CreateKVClientTable<double>(table_id);

        LOG(INFO) << "Worker " << info.worker_id << " got " << node_samples.size() << " samples.";

        //std::cout << "Initializing keys" << std::endl;
        // Initialize all keys with 0
        std::vector<Key> keys;
        for (int i=0; i<FLAGS_n_features+1; i++) keys.push_back(i); 
        std::vector<double> init_vals(FLAGS_n_features+1);          
        table.Add(keys, init_vals);
        //std::cout << "Key Initialized" << std::endl;

        // Train
        std::vector<long long> m_times;
        std::vector<long long> m_wait_times;
        
        double learning_rate = FLAGS_alpha;


        std::vector<unsigned int> indices(node_samples.size());
        std::iota(indices.begin(), indices.end(), 0);
        
        for (int i = 0 ; i < FLAGS_n_iters; i++ )
        // for (Sample sample : node_samples)
        {
            auto iter_start_time = std::chrono::steady_clock::now();
            
            // Pull
            std::vector<double> vals;
            
            auto iter_wait_time_1 = std::chrono::steady_clock::now();
            table.Get(keys, &vals);
            auto iter_wait_time_2 = std::chrono::steady_clock::now();
            // std::cout << "Got vals: ";
            // for (auto val : vals) std::cout << val << ' ';
            // std::cout << std::endl;

            std::random_shuffle(indices.begin(), indices.end());
            
              // Pick a sample randomly
            for (int b = 0 ; b < FLAGS_batch_size ; b++ )
            {
              auto sample = node_samples[indices[ b % node_samples.size() ]];
            
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
            }

            // Push
            //std::cout << "Adding vals: ";
            //for (auto val : vals) std::cout << val << ' ';
            //std::cout << std::endl;
            table.Add(keys, vals);
            table.Clock();

            if ((i % (FLAGS_n_iters/10)) == 0)
            {
                auto sum = std::accumulate(m_times.begin(), m_times.end(), 0);
                auto m_mean = sum / (FLAGS_n_iters/10);
                
                std::vector<long long> diff( FLAGS_n_iters/10 );
                std::transform(m_times.begin(), m_times.end(), diff.begin(),
                               [m_mean](long long t) {return t - m_mean;});

                auto sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0);
                auto m_st_dev = std::sqrt(sq_sum / (FLAGS_n_iters/10));

                auto wait_sum = std::accumulate(m_wait_times.begin(), m_wait_times.end(), 0);
                auto m_wait_mean = wait_sum / (FLAGS_n_iters/10);
                
                std::vector<long long> wait_diff( FLAGS_n_iters/10 );
                std::transform(m_wait_times.begin(), m_wait_times.end(), wait_diff.begin(),
                               [m_wait_mean](long long t) {return t - m_wait_mean;});

                auto wait_sq_sum = std::inner_product(wait_diff.begin(), wait_diff.end(), wait_diff.begin(), 0);
                auto m_wait_st_dev = std::sqrt(wait_sq_sum / (FLAGS_n_iters/10));

                
                LOG(INFO) << "Worker " << info.worker_id << " for " << std::setw(8) << i << " iterations" 
                                  << " total: " << std::setw(2) << sum << "ms"
                                  << " mean: " << std::setw(3) << m_mean << "ms"
                                  << " st. dev: : " << std::setw(6) << m_st_dev 
                                  << " wait: " << std::setw(2) << wait_sum << "ms"
                                  << " mean: " << std::setw(3) << m_wait_mean << "ms"
                                  << " st. dev: : " << std::setw(6) << m_wait_st_dev;
                m_times.clear();
                m_wait_times.clear();

            } else {
                auto time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - iter_start_time).count();
                auto wait_time = std::chrono::duration_cast<std::chrono::microseconds>(iter_wait_time_2 - iter_wait_time_1).count();
                m_times.push_back(time);
                m_wait_times.push_back(wait_time);
            } 
        }

        // Test
        std::vector<double> vals;
        table.Get(keys, &vals);

        //std::cout << "Printing out first 10 coeff:" << std::endl;
        //for (int i=0; i<10; i++) std::cout << vals.at(i) << ' ';
        //std::cout << std::endl;

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
        LOG(INFO) << "Accuracy: " << correct << " out of " << test_samples.size() 
                  << " " << float(correct)/test_samples.size()*100 << " percent"; 

#ifdef BENCHMARK
        }, info); LOG(INFO) << "Worker " << info.worker_id << " total runtime: " << benchmark << "ms";
#endif
    });

    engine->Run(task);
    // engine->Barrier();
    engine->StopEverything();
   
    return 0;
}
