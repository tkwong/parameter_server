#include <thread>
#include <iostream>
#include <math.h>
#include <random>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "lib/labeled_sample.hpp"
#include "lib/abstract_data_loader.hpp"
#include "driver/engine.hpp"
#include "lib/parser.hpp"

using namespace csci5570;

typedef double Sample;
typedef std::vector<Sample> DataStore;

DEFINE_string(config_file, "", "The config file path");
DEFINE_string(input, "", "The hdfs input url");

int main(int argc, char** argv)
{
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    FLAGS_stderrthreshold = 0;
    FLAGS_colorlogtostderr = true;


    LOG(INFO) << FLAGS_config_file;

    Node node{0, "localhost", 12353};
    Engine engine(node, {node});
    // start
    engine.StartEverything();
    const auto kDataTableId = engine.CreateTable<Sample>(ModelType::BSP, StorageType::Map);  // table 1
    
    const auto kTableId = engine.CreateTable<double>(ModelType::SSP, StorageType::Map);  // table 0

    engine.Barrier();
    
    MLTask loadDataTask; 
    loadDataTask.SetWorkerAlloc({{0,3}});
    loadDataTask.SetTables({kDataTableId});
    loadDataTask.SetLambda([kDataTableId](const Info& info) {
      LOG(INFO) << "Data Loading Task";
      LOG(INFO) << info.DebugString();
      auto datatable = info.CreateKVClientTable<Sample>(kDataTableId);

      
      // FIXME: Parse the sample data, now using random data.
      
      // std::random_device rd; // obtain a random number from hardware
      // std::mt19937 eng(rd()); // seed the generator
      // std::uniform_int_distribution<std::mt19937::result_type> distr(1, 123); // define the range
      //
      // Sample sample1;
      // for(int n=0; n<distr(eng); ++n)
      //   sample1 += (distr(eng));; // generate numbers
      // 
      Sample sample1(999);
      
      // Use WorkerID as the Key
      std::vector<Key> keys{0};
      // Use Sample as Value
      std::vector<Sample> vals{sample1};
      //
      LOG(INFO) << "Add Data to kDataTableId : <" << keys[0] ;
      datatable.Add(keys, vals);
      LOG(INFO) << "Clock to kDataTableId ";
      datatable.Clock();
      
      // std::vector<Sample> ret;
      // datatable.Get(keys, &ret);
      // for (auto r_ : ret )
      //   LOG(INFO) << "GOT Sample : "<< r_;
      
      
    });
    LOG(INFO) << "Start Loading Data Task";
    engine.Run(loadDataTask);
    
    MLTask task;
    task.SetWorkerAlloc({{0, 3}});  // 3 workers on node 0
    task.SetTables({kTableId, kDataTableId});     // Use table 0
    task.SetLambda([kTableId, kDataTableId](const Info& info) {
      LOG(INFO) << "Hi";
      LOG(INFO) << info.DebugString();
      // LOG(INFO) << info.partition_manager_map.find(kTableId) ;
      auto datatable = info.CreateKVClientTable<Sample>(kDataTableId);
      std::vector<Key> keys{info.worker_id};
      std::vector<Sample> ret;
      LOG(INFO) << "Try Get : "<< keys[0];
      datatable.Get(keys, &ret);
      for (auto r_ : ret) {
        LOG(INFO) << "GOT Sample : "<< r_;
      }
      
      
      // KVClientTable<double> table = info.CreateKVClientTable<double>(kTableId);
      // for (int i = 0; i < 5; ++i) {
      //   std::vector<Key> keys{1};
      //   std::vector<double> vals{0.5};
      //   table.Add(keys, vals);
      //   std::vector<double> ret;
      //   table.Get(keys, &ret);
      //   LOG(INFO) << ret[0];
      // }
    });
    engine.Run(task);

    // stop
    engine.StopEverything();
    return 0;
}
