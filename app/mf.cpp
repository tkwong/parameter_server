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
DEFINE_int32(n_users, -1, "The number of users in the dataset");
DEFINE_int32(n_items, -1, "The number of items in the dataset");
// Training config
DEFINE_int32(n_workers_per_node, 1, "The number of loaders per node");
DEFINE_int32(n_iters, 10, "The number of iterations");
DEFINE_int32(batch_size, 100, "Batch size");
DEFINE_double(alpha, 0.0001, "learning rate");
DEFINE_double(beta, 0.02, "regularization parameter");
DEFINE_int32(latent, 10, "number of latent dimensions");

DEFINE_double(with_injected_straggler, 0.0, "injected straggler with its probability");
DEFINE_double(with_injected_straggler_delay, 100, "injected straggler delay in ms (obsoleted)");
DEFINE_double(with_injected_straggler_delay_percent, 100, "injected straggler delay in %");
DEFINE_int32(get_updated_workload_rate, 5, "the rate (in iterations) to update the workload size from table.");
DEFINE_bool(activate_scheduler, true, "activate scheduler");
DEFINE_bool(activate_transient_straggler, false, "activate transient straggler");
DEFINE_bool(activate_permanent_straggler, false, "activate permanemt straggler");
DEFINE_bool(prefetch_model_before_batch, false, "Prefetch (Get) needed model data before each batch");

// DEFINE_validator(my_id, [](const char* flagname, int value){ return value>=0;});
// DEFINE_validator(config_file, [](const char* flagname, std::string value){ return false;});
DEFINE_validator(input, [](const char* flagname, const std::string& value){ return !value.empty();});
DEFINE_validator(n_users, [](const char* flagname, int value){ return value>=0;});
DEFINE_validator(n_items, [](const char* flagname, int value){ return value>=0;});

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
        FLAGS_input, 2,
        lib::Parser<Sample, DataStore>::parse_matrix, &samples, 0, nodes.size(), hdfs_master_host, FLAGS_hdfs_master_port, my_node->hostname
    );

    DataStore node_samples(samples.begin() , samples.end());
    // DataStore test_samples(samples.end() - 10000, samples.end());

    LOG(INFO) << "Got " << node_samples.size() << " samples";

    Sample sample = *(samples.begin());
    LOG(INFO) << "First sample: " << sample.features.coeffRef(0) << " " << sample.features.coeffRef(1) << " " << sample.label;

    Engine* engine = new Engine(*my_node, nodes);
    engine->StartEverything();

    auto user_bias_table_id = engine->CreateTable<double>(ModelType::BSP, StorageType::Map, 3);
    auto item_bias_table_id = engine->CreateTable<double>(ModelType::BSP, StorageType::Map, 3);
    auto user_matrix_table_id = engine->CreateTable<double>(ModelType::BSP, StorageType::Map, 3);
    auto item_matrix_table_id = engine->CreateTable<double>(ModelType::BSP, StorageType::Map, 3);
    engine->Barrier();

    MLTask task;
    std::vector<WorkerAlloc> worker_alloc;
    for (auto& node : nodes) {
      worker_alloc.push_back({node.id, static_cast<uint32_t>(FLAGS_n_workers_per_node)});
    }
    // task.SetWorkerAlloc({{0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3}});
    task.SetWorkerAlloc(worker_alloc);
    task.SetTables({user_bias_table_id, item_bias_table_id, user_matrix_table_id, item_matrix_table_id});

    // Scheduling Task
    task.SetScheduler([](const Info& info)
    {
      // Tunning Parameters
      const double update_threshold   = 1.5; // min-max time diff in factor
      const double rebalance_workload = 0.2; // percentage

      LOG(INFO) << "Worker id: " << info.worker_id << " is scheduler";

      std::vector<int> workloads(info.thread_ids.size(), FLAGS_batch_size);
      info.workloadTable->Add(info.thread_ids, workloads);

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
              VLOG(2) << "[STAT_TIME] " << i << "," << info.thread_ids[j] << ","
                        << times[j];


          // Scheduling algorithm
          if (FLAGS_activate_scheduler && i % (FLAGS_get_updated_workload_rate + 1) == 0)
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
              VLOG(2) << "[STAT_WORK] " << i << "," << info.thread_ids[j] << ","
                        << workloads[j];
      }

      LOG(INFO) << "Scheduler terminating";
      return;
    });

    task.SetLambda([user_bias_table_id, item_bias_table_id, user_matrix_table_id, item_matrix_table_id, node_samples](const Info& info)
    {
#ifdef BENCHMARK
        Benchmark<> benchmark_woker_time;
        benchmark_woker_time.start_measure();
#endif

        //LOG(INFO) << "Worker id: " << info.worker_id << " table id: " << table_id;

        // KVClientTable<double> table = info.CreateKVClientTable<double>(table_id);
        auto user_bias_table = info.CreateKVClientTable<double>(user_bias_table_id);
        auto item_bias_table = info.CreateKVClientTable<double>(item_bias_table_id);
        auto user_matrix_table = info.CreateKVClientTable<double>(user_matrix_table_id);
        auto item_matrix_table = info.CreateKVClientTable<double>(item_matrix_table_id);

        LOG(INFO) << "Worker " << info.worker_id << " got " << node_samples.size() << " samples";

        // Estimate mean rating
        int mean_rating = 0;
        Eigen::SparseMatrix<int> full_matrix(FLAGS_n_users, FLAGS_n_items);
        for (int pos = 0; pos < std::min<int>(node_samples.size(), 5000); pos++)
        {
            auto sample = node_samples[pos];
            int i = sample.features.coeffRef(0);
            int j = sample.features.coeffRef(1);
            full_matrix.coeffRef(i, j) = sample.label;
            mean_rating += sample.label;
        }
        mean_rating /= node_samples.size();


        // Train
#ifdef BENCHMARK       
        Benchmark<> benchmark_iteration;
        Benchmark<> benchmark_batch;
        Benchmark<> benchmark_iter_process_time;
        Benchmark<> benchmark_wait_time;
#endif
        Benchmark <> actual_process_timer_;

        std::vector<unsigned int> indices(node_samples.size());
        std::iota(indices.begin(), indices.end(), 0);

        int batch_size = FLAGS_batch_size;

        LOG(INFO) << "Start iteration...  [" << info.worker_id  << "]";
        for (int i = 0 ; i < FLAGS_n_iters; i++ )
        {
#ifdef BENCHMARK
            benchmark_iteration.start_measure();
#endif

            if (FLAGS_activate_scheduler && info.scheduler_id != 0 && i % FLAGS_get_updated_workload_rate == 0)
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

            LOG(INFO) << "Running batch "<< batch_size << "...  [" << info.worker_id  << "]";
            // Pick a sample randomly from sample indices
            
            // Get key value for this batch size (for example: batch = 10)
            std::map <Key, double> user_bias_kvs, item_bias_kvs, user_matrix_kvs, item_matrix_kvs;
            /*if (FLAGS_prefetch_model_before_batch)
            {
                // _kvs[0] = 0;
                for (int b = 0 ; b < batch_size ; b++ )
                {
                  auto sample = node_samples[indices[ b % node_samples.size() ]];
                  //for (Eigen::SparseVector<double>::InnerIterator it(sample.features); it; ++it)
                  //{
                      //DLOG(INFO) << "it.index(): " << it.index() ;
                  _user_kvs[sample.features.coeffRef(0)] = 0 ;
                  _item_kvs[sample.features.coeffRef(1)] = 0;
                  //}
                }

                std::vector<Key> _user_keys, _item_keys;
                std::vector<double> _user_bias_vals, _item_bias_vals, _user_matrix;              
                for(auto const& kv: _user_kvs) _user_keys.push_back(kv.first);
                for(auto const& kv: _item_kvs) _item_keys.push_back(kv.first);
#ifdef BENCHMARK
                  benchmark_wait_time.start_measure();
#endif

                //table.Get(_keys, &_vals);

                //CHECK_EQ(_keys.size() ,  _vals.size());

#ifdef BENCHMARK
                  // [STAT_GET]<iteration>,<thread_id>,<wait_time>
                  benchmark_wait_time.stop_measure();
                  VLOG(2) << "[STAT_WAIT] " << i << "," << info.thread_id <<  "," << benchmark_wait_time.last();
#endif

                for(int i = 0; i < _keys.size() ; i++)
                {
                    // DLOG(INFO) << _keys[i] << " = " << _vals[i] ;
                   //_kvs[_keys[i]] = _vals[i];
                }

            }*/
            
            for (int b = 0 ; b < batch_size ; b++ )
            {

              auto sample = node_samples[indices[ b % node_samples.size() ]];

              // Prepare the keys from sample
              std::vector<Key> user_bias_keys, item_bias_keys, user_matrix_keys, item_matrix_keys;
              user_bias_keys.push_back(sample.features.coeffRef(0));
              item_bias_keys.push_back(sample.features.coeffRef(1));
              for (int l = 0; l < FLAGS_latent; l++)
              {
                  // the table stores the matrix as flatten vector
                  user_matrix_keys.push_back(sample.features.coeffRef(0) + l * FLAGS_n_users); 
                  item_matrix_keys.push_back(sample.features.coeffRef(1) + l * FLAGS_n_items);
              }


              // Get Vals
              std::vector<double> user_bias_vals, item_bias_vals, user_matrix_vals, item_matrix_vals;              

              if (FLAGS_prefetch_model_before_batch)
              {
                  // Get the vals from prefetched kv store
                  //for(int i = 0; i < keys.size() ; i++)
                  //{
                    //CHECK_NE(_kvs.find(keys[i]), _kvs.end());
                    //vals.push_back(_kvs[keys[i]]);
                  //}
              } 
              else 
              { 

#ifdef BENCHMARK
                  benchmark_wait_time.start_measure();
#endif
                // Get the key that not in _kvs
                //std::vector<Key> keys_;
                //std::vector<double> vals_;
                //for(int i = 0; i < keys.size() ; i++)
                //    if(_kvs.find(keys[i]) == _kvs.end())
                //        keys_.push_back(keys[i]);

                // Do Get for the missing key val 
                // table.Get(keys_, &vals_);
                user_bias_table.Get(user_bias_keys, &user_bias_vals);
                item_bias_table.Get(item_bias_keys, &item_bias_vals);
                user_matrix_table.Get(user_matrix_keys, &user_matrix_vals);
                item_matrix_table.Get(item_matrix_keys, &item_matrix_vals);

                //cache the key
                //for(int i = 0 ; i < keys_.size() ; i++)
                //    _kvs[keys_[i]] = vals_[i];
                
                // set the vals from the cache (should be filled by previous get)
                //for(int i = 0; i < keys.size() ; i++)
                //{
                    //CHECK_NE(_kvs.find(keys[i]), _kvs.end());
                //    vals.push_back(_kvs[keys[i]]);
                //}

#ifdef BENCHMARK
                  // [STAT_GET]<iteration>,<thread_id>,<wait_time>
                  benchmark_wait_time.stop_measure();
                  VLOG(2) << "[STAT_WAIT] " << i << "," << info.thread_id <<  "," << benchmark_wait_time.last();
#endif

              }

#ifdef BENCHMARK
              benchmark_iter_process_time.start_measure();
#endif
              // Measure Actual Process Time 
              actual_process_timer_.start_measure();

              // DLOG(INFO) << "vals.size(): " << vals.size();
              // CHECK_EQ(keys.size(), vals.size());

              // Predict
              Eigen::VectorXf user_vec(user_matrix_vals.size()), item_vec(item_matrix_vals.size());
              for (int i = 0; i < user_matrix_vals.size(); i++) 
              {
                  if (user_matrix_vals[i] == 0)
                      user_vec[i] = 0.01;
                  else
                      user_vec[i] = user_matrix_vals[i];
              }
              for (int i = 0; i < item_matrix_vals.size(); i++) 
              {
                  if (item_matrix_vals[i] == 0)
                      item_vec[i] = 0.01;
                  else
                      item_vec[i] = item_matrix_vals[i];
              }
              double predict = mean_rating + user_bias_vals[0] + item_bias_vals[0] + user_vec.dot(item_vec);
              LOG(INFO) << "predict: " << predict;

              // Cal Error and Gradient
              double error = sample.label - predict;
              LOG(INFO) << "error: " << error; 

              LOG(INFO) << "alpha: " << FLAGS_alpha << " beta: " << FLAGS_beta;

              // Update
              LOG(INFO) << "old user_bias: " << user_bias_vals[0] << " item_bias: " << item_bias_vals[0];

              user_bias_vals[0] += FLAGS_alpha * (error - FLAGS_beta * user_bias_vals[0]);
              item_bias_vals[0] += FLAGS_alpha * (error - FLAGS_beta * item_bias_vals[0]);

              LOG(INFO) << "new user_bias: " << user_bias_vals[0] << " item_bias: " << item_bias_vals[0];

              LOG(INFO) << "old user matrix : " << user_vec.transpose();
              LOG(INFO) << "old item matrix : " << item_vec.transpose();

              user_vec += FLAGS_alpha * (error * item_vec - FLAGS_beta * user_vec);
              item_vec += FLAGS_alpha * (error * user_vec - FLAGS_beta * item_vec);

              for (int i = 0; i < user_matrix_vals.size(); i++) user_matrix_vals[i] = user_vec[i];
              for (int i = 0; i < item_matrix_vals.size(); i++) item_matrix_vals[i] = item_vec[i];

              LOG(INFO) << "new user matrix : " << user_vec.transpose();
              LOG(INFO) << "new item matrix : " << item_vec.transpose(); 

              actual_process_timer_.stop_measure();

              // Transient Straggler
              if (FLAGS_with_injected_straggler > 0.0 ) {
                double r = (double)rand() / RAND_MAX;
                if (r < FLAGS_with_injected_straggler) {
                  double delay = actual_process_timer_.last() * (FLAGS_with_injected_straggler_delay_percent / 100);
                  LOG(INFO) << "[" << info.worker_id  << "] injecting straggler for " << int(delay) << " ms ... [" << r << "]";
                  std::this_thread::sleep_for(std::chrono::milliseconds(int(delay)));
                }
              }

              // Permanent Straggler
              if (FLAGS_activate_permanent_straggler && info.worker_id == 3)
              {
                  double delay = actual_process_timer_.last() * (FLAGS_with_injected_straggler_delay_percent / 100);
                  std::this_thread::sleep_for(std::chrono::milliseconds(int(delay)));
              }

              // Simple Transient Straggler
              if (FLAGS_activate_transient_straggler && info.worker_id == 1 && i >= 50 && i < 75)
              {
                  double delay = actual_process_timer_.last() * (FLAGS_with_injected_straggler_delay_percent / 100);
                  std::this_thread::sleep_for(std::chrono::milliseconds(int(delay)));
              }

              // Push
              //table.Add(keys, vals);
              user_bias_table.Add(user_bias_keys, user_bias_vals);
              item_bias_table.Add(item_bias_keys, item_bias_vals);
              user_matrix_table.Add(user_matrix_keys, user_matrix_vals);
              item_matrix_table.Add(item_matrix_keys, item_matrix_vals);


#ifdef BENCHMARK
              // [STAT_PROCESS]<iteration>,<thread_id>,<process_time>
              benchmark_iter_process_time.stop_measure();
              VLOG(2) << "[STAT_PROC] " << i << "," << info.thread_id << "," << benchmark_iter_process_time.last() << "," << b;

#endif

            }

            // LOG(INFO) << "Finished batch "<< FLAGS_batch_size << " [" << info.worker_id  << "]";

            //table.Clock();
            user_bias_table.Clock();
            item_bias_table.Clock();
            user_matrix_table.Clock();
            item_matrix_table.Clock();

#ifdef BENCHMARK
            info.reportTime(benchmark_iter_process_time.last(batch_size));

            // [STAT_ITER]<iteration>,<thread_id>,<iteration_time>
            VLOG(2) << "[STAT_ITER] " << i << "," << info.thread_id << "," << benchmark_iteration.stop_measure();

            LOG_EVERY_N(INFO, (FLAGS_n_iters/10)) << "Worker " << info.worker_id  << " for " << i << " Iterations. "
                              << "iteration: "        << benchmark_iteration.mean()/1000. << " ms "
                              << "batch: "            << benchmark_batch.mean()/1000. << " ms "
                              << "process_time: "     << benchmark_iter_process_time.mean()/1000. << " ms "
                              << "wait_time: "        << benchmark_wait_time.mean()/1000. << " ms ";
              
#endif
            
        }

        LOG(INFO) << "Finished iteration...  [" << info.worker_id  << "]";
        
#ifdef BENCHMARK
        benchmark_woker_time.stop_measure();
        LOG(INFO) << "Worker " << info.worker_id << " total runtime: " << benchmark_woker_time.sum()/1000. << "ms";
        VLOG(2) << "[STAT_TOTL] " << info.thread_id << "," << benchmark_woker_time.sum();
#endif

        // Test
        double error = 0;
        for (int col = 0; col < full_matrix.outerSize(); ++col)
        {
            for (Eigen::SparseMatrix<int>::InnerIterator it(full_matrix, col); it; ++it)
            {

                std::vector<Key> user_bias_keys, item_bias_keys, user_matrix_keys, item_matrix_keys;
                user_bias_keys.push_back(it.index());
                item_bias_keys.push_back(col);
                for (int l = 0; l < FLAGS_latent; l++)
                {
                    // the table stores the matrix as flatten vector
                    user_matrix_keys.push_back(it.index() + l * FLAGS_n_users); 
                    item_matrix_keys.push_back(col        + l * FLAGS_n_items);
                } 

                std::vector<double> user_bias_vals, item_bias_vals, user_matrix_vals, item_matrix_vals;              

                user_bias_table.Get(user_bias_keys, &user_bias_vals);
                item_bias_table.Get(item_bias_keys, &item_bias_vals);
                user_matrix_table.Get(user_matrix_keys, &user_matrix_vals);
                item_matrix_table.Get(item_matrix_keys, &item_matrix_vals);

                LOG(INFO) << "it.index(): " << it.index() << " col: " << col << " user_bias: " << user_bias_vals[0]
                          << " item_bias: " << item_bias_vals[0];
 
                Eigen::VectorXf user_vec(user_matrix_vals.size()), item_vec(item_matrix_vals.size());
                for (int i = 0; i < user_matrix_vals.size(); i++) user_vec[i] = user_matrix_vals[i];
                for (int i = 0; i < item_matrix_vals.size(); i++) item_vec[i] = item_matrix_vals[i];
                double predict = mean_rating + user_bias_vals[0] + item_bias_vals[0] + user_vec.dot(item_vec);

                error += pow(it.value() - predict, 2);
            }
        }
        LOG(INFO) << "Error: " << sqrt(error);
    });

    engine->Run(task);
    engine->Barrier();
    engine->StopEverything();

    return 0;
}
