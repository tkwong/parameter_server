#include <thread>
#include <iostream>
#include <math.h>
#include <numeric>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>

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

DEFINE_double(with_injected_straggler, 0.0, "injected straggler with its probability");
DEFINE_double(with_injected_straggler_delay, 100, "injected straggler delay in ms");
DEFINE_int32(get_updated_workload_rate, 5, "the rate (in iterations) to update the workload size from table.");
DEFINE_bool(activate_scheduler, true, "activate scheduler");
DEFINE_bool(activate_transient_straggler, true, "activate transient straggler");
DEFINE_bool(activate_permanent_straggler, true, "activate permanemt straggler");

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
        lib::Parser<Sample, DataStore>::parse_libsvm, &samples, my_node->id, nodes.size(), hdfs_master_host, FLAGS_hdfs_master_port, my_node->hostname
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

    // Scheduling Task
    task.SetScheduler([table_id, node_samples, test_samples](const Info& info)
    {
      // Tunning Parameters
      const double update_threshold   = 1.5; // min-max time diff in factor
      const double rebalance_workload = 0.2; // percentage

      LOG(INFO) << "Worker id: " << info.worker_id << " is scheduler";

      std::vector<int> workloads(info.thread_ids.size(), FLAGS_batch_size);
      info.workloadTable->Add(info.thread_ids, workloads);

      if (! FLAGS_activate_scheduler) 
      {
          LOG(INFO) << "Scheduler not activated, thread " << info.thread_id 
                    << " terminating.";
          return;
      }

      for (int i = 0; i < FLAGS_n_iters; i++)
      {
          LOG(INFO) << "===== Iteration " << i << " =====";

          info.timeTable->Clock();
          std::vector<double> times;
          info.timeTable->Get(info.thread_ids, &times);

          std::stringstream tstream;
          tstream << std::endl;
          tstream << "-------------- TimeTable ---------------" << std::endl;
          for (int j = 0; j < info.thread_ids.size(); j++)
              tstream << "| Thread id: " << info.thread_ids[j] << " | Iter time: "
                      << std::setw(7) << times[j] << " |" << std::endl;
          tstream << "----------------------------------------" << std::endl;
          // LOG(INFO) << tstream.str();

          // [STAT_TIME]<iteration>,<thread_id>,<process_time>
          for (int j = 0; j < info.thread_ids.size(); j++)
              LOG(INFO) << "[STAT_TIME]" << i << "," << info.thread_ids[j] << ","
                        << times[j];


          // Scheduling algorithm
          if (i % (FLAGS_get_updated_workload_rate + 1) == 0)
          {
              double min_time = *std::min_element(times.begin(), times.end());
              int workload_buffer = 0, count = 0;

              std::multimap<double, uint32_t> time2thread;
              std::map<uint32_t, int> thread2index;

              for (int j = 0; j < info.thread_ids.size(); j++)
              {
                  if (times[j] > min_time * update_threshold)
                  {
                      workload_buffer += round(workloads[j] * rebalance_workload);
                      workloads[j] = round(workloads[j] * (1 - rebalance_workload));
                  }
                  else count += 1;

                  time2thread.insert(std::make_pair(times[j], info.thread_ids[j]));
                  thread2index.insert(std::make_pair(info.thread_ids[j], j));
              }

              for (auto pair : time2thread)
              {
                  workloads.at(thread2index.at(pair.second)) += round(workload_buffer / double(count));
                  workload_buffer -= round(workload_buffer / double(count));
                  count -= 1;
                  if (workload_buffer < 1) break;
              }

              info.workloadTable->Add(info.thread_ids, workloads); 
          }

          std::stringstream wstream;
          wstream << std::endl;
          wstream << "------------ WorkloadTable ------------" << std::endl;
          for (int j = 0; j < info.thread_ids.size(); j++)
              wstream << "| Thread id: " << info.thread_ids[j] << " | Batch size: "
                      << std::setw(5) << workloads[j] << " |" << std::endl;
          wstream << "---------------------------------------" << std::endl;
          // LOG(INFO) << wstream.str();

          // [STAT_WORKLOAD]<iteration>,<thread_id>,<batch_size>
          for (int j = 0; j < info.thread_ids.size(); j++)
              LOG(INFO) << "[STAT_WORKLOAD]" << i << "," << info.thread_ids[j] << ","
                        << workloads[j];
      }

      LOG(INFO) << "Scheduler terminating";
      return;
    });

    task.SetLambda([table_id, node_samples, test_samples](const Info& info)
    {
#ifdef BENCHMARK
        auto benchmark = Benchmark<>::measure([&](const Info& info) {
#endif

        LOG(INFO) << "Worker id: " << info.worker_id << " table id: " << table_id;

        KVClientTable<double> table = info.CreateKVClientTable<double>(table_id);

        LOG(INFO) << "Worker " << info.worker_id << " got " << node_samples.size() << " samples";

        // Train
#ifdef BENCHMARK       
        Benchmark<> benchmark_iteration;
        Benchmark<> benchmark_batch;
        Benchmark<> benchmark_iter_process_time;
        Benchmark<> benchmark_wait_time;
#endif

        double learning_rate = FLAGS_alpha;

        std::vector<unsigned int> indices(node_samples.size());
        std::iota(indices.begin(), indices.end(), 0);

        int batch_size = FLAGS_batch_size;

        LOG(INFO) << "Start iteration...  [" << info.worker_id  << "]";
        for (int i = 0 ; i < FLAGS_n_iters; i++ )
        {
#ifdef BENCHMARK
            benchmark_iteration.start_measure();
#endif

            if (i % FLAGS_get_updated_workload_rate == 0)
            {
                batch_size = info.getWorkload();
                LOG(INFO) << "Worker " << info.worker_id
                          << " workload for the next "
                          << FLAGS_get_updated_workload_rate
                          << "  iterations is "
                          << batch_size ;
            }

            // Prepare Sample indices
            std::random_shuffle(indices.begin(), indices.end());

            // LOG(INFO) << "Start batch "<< FLAGS_batch_size << "...  [" << info.worker_id  << "]";
            // Pick a sample randomly from sample indices
            
            
            //Get key for this batch size (for example: batch = 10)
            for (int b = 0 ; b < batch_size ; b++ )
            {
#ifdef BENCHMARK
              benchmark_batch.start_measure();
#endif
              auto sample = node_samples[indices[ b % node_samples.size() ]];

              // Prepare the keys from sample
              std::vector<Key> keys;
              keys.push_back(0);
              for (Eigen::SparseVector<double>::InnerIterator it(sample.features); it; ++it){
                // DLOG(INFO) << "it.index(): " << it.index() ;
                keys.push_back(it.index()) ;
              }


              // Get Vals
#ifdef BENCHMARK
              benchmark_wait_time.start_measure();
#endif

              std::vector<double> vals;              
              table.Get(keys, &vals);
              
#ifdef BENCHMARK
              // [STAT_GET]<iteration>,<thread_id>,<wait_time>
              LOG(INFO) << "[STAT_GET]" << i << "," << info.thread_id <<  "," << benchmark_wait_time.stop_measure();
              // benchmark_wait_time.stop_measure();
#endif
              
              
#ifdef BENCHMARK
              benchmark_iter_process_time.start_measure();
#endif
              // DLOG(INFO) << "vals.size(): " << vals.size();
              CHECK_EQ(keys.size(), vals.size());

              // Predict
              double yhat = vals.at(0);
              // starting from 1, as the position of key = position of val
              for (int i = 1 ; i < keys.size() ; i ++ ) {
                  yhat += vals.at(i) * sample.features.coeffRef(keys[i]);
              }


              double predict = 1.0 / (1.0 + exp(-yhat));

              // Cal Error and Gradient
              double error = (sample.label > 0 ? 1 : 0) - predict;
              double gradient = vals.at(0);

              for (Eigen::SparseVector<double>::InnerIterator it(sample.features); it; ++it)
                  gradient += it.value() * error;

              // Update
              vals.at(0) += learning_rate * gradient;
              for (int i = 1 ; i < keys.size(); i++ )
                  vals.at(i) += learning_rate * gradient;

              // Transient Straggler
              if (FLAGS_with_injected_straggler > 0.0 ) {
                double r = (double)rand() / RAND_MAX;
                if (r < FLAGS_with_injected_straggler) {
                  double delay = FLAGS_with_injected_straggler_delay;
                  LOG(INFO) << "[" << info.worker_id  << "] injecting straggler for " << int(delay) << " ms ... [" << r << "]";
                  std::this_thread::sleep_for(std::chrono::milliseconds(int(delay)));
                }
              }

              // Permanent Straggler
              if (FLAGS_activate_permanent_straggler && info.worker_id == 3)
              {
                  std::this_thread::sleep_for(std::chrono::milliseconds(int(FLAGS_with_injected_straggler_delay)));
              }

              // Simple Transient Straggler
              if (FLAGS_activate_transient_straggler && info.worker_id == 1 && i >= 50 && i < 75)
              {
                  std::this_thread::sleep_for(std::chrono::milliseconds(int(FLAGS_with_injected_straggler_delay)));
              }

              // Push
              table.Add(keys, vals);
              
#ifdef BENCHMARK
              benchmark_batch.stop_measure();

              // [STAT_PROCESS]<iteration>,<thread_id>,<process_time>
              LOG(INFO) << "[STAT_PROCESS] " << i << "," << info.thread_id << "," << benchmark_iter_process_time.stop_measure();
              // benchmark_iter_process_time.stop_measure();
#endif

            }

            // LOG(INFO) << "Finished batch "<< FLAGS_batch_size << " [" << info.worker_id  << "]";

            table.Clock();

#ifdef BENCHMARK
            // [STAT_ITER]<iteration>,<thread_id>,<iteration_time>
            LOG(INFO) << "[STAT_ITER]" << i << "," << info.thread_id << "," << benchmark_iteration.stop_measure();

            info.reportTime(benchmark_iter_process_time.last(batch_size));

            if ((i % (FLAGS_n_iters/10)) == 0)
            {              
              LOG(INFO) << "Worker " << info.worker_id  << " for " << i << " Iterations. "
                                << "iteration: "        << benchmark_iteration.mean()/1000. << " ms "
                                << "batch: "            << benchmark_batch.mean()/1000. << " ms "
                                << "process_time: "     << benchmark_iter_process_time.mean()/1000. << " ms "
                                << "wait_time: "        << benchmark_wait_time.mean()/1000. << " ms ";
                                  
            }
#endif
        }


        LOG(INFO) << "Finished iteration...  [" << info.worker_id  << "]";

        // Test

        int correct = 0;
        for (Sample sample : test_samples)
        {
            std::vector<double> vals;
            // Prepare the keys from sample
            std::vector<Key> keys;
            for (Eigen::SparseVector<double>::InnerIterator it(sample.features); it; ++it)
              keys.push_back(it.index()) ;

            table.Get(keys, &vals);

            double yhat = vals.at(0);
            for (int i = 1 ; i < keys.size() ; i ++ ) {
                yhat += vals.at(i) * sample.features.coeffRef(keys[i]);
            }
            double estimate = 1.0 / (1.0 + exp(-yhat));

            int est_class = (estimate > 0.5 ? 1 : 0);
            int answer = (sample.label > 0 ? 1 : 0);
            if (est_class == answer) correct++;
        }
        LOG(INFO) << "Accuracy: " << correct << " out of " << test_samples.size()
                  << " " << float(correct)/test_samples.size()*100 << " percent";

#ifdef BENCHMARK
        }, info); LOG(INFO) << "Worker " << info.worker_id << " total runtime: " << benchmark/1000. << "ms";
#endif
    });

    engine->Run(task);
    engine->Barrier();
    engine->StopEverything();

    return 0;
}
