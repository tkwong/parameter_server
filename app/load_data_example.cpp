#include <thread>
#include <iostream>
#include <math.h>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "lib/labeled_sample.hpp"
#include "lib/abstract_data_loader.hpp"
#include "driver/engine.hpp"
#include "lib/parser.hpp"

using namespace csci5570;

typedef lib::LabeledSample<int, double> Sample;
typedef std::vector<Sample> DataStore;
typedef int TrainingData;

int main(int argc, char** argv)
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_stderrthreshold = 0;
    FLAGS_colorlogtostderr = true;

    DataStore train_data;

    Node node{0, "localhost", 12353};
    Engine engine(node, {node});
    // start
    engine.StartEverything();
    const auto kDataTableId = engine.CreateTable<TrainingData>(ModelType::BSP, StorageType::Vector);  // table 1
    
    const auto kTableId = engine.CreateTable<double>(ModelType::SSP, StorageType::Map);  // table 0

    engine.Barrier();
    
    MLTask loadDataTask; 
    loadDataTask.SetWorkerAlloc({{0,3}});
    loadDataTask.SetTables({kDataTableId});
    loadDataTask.SetLambda([kDataTableId](const Info& info) {
      LOG(INFO) << "Data Loading Task";
      LOG(INFO) << info.DebugString();
      auto datatable = info.CreateKVClientTable<TrainingData>(kDataTableId);
      
      boost::string_ref line1 = "-1 35:1 48:1 70:1 149:1 250:1";
      
      // auto sample1 = csci5570::lib::Parser<Sample, DataStore>::parse_libsvm(line1, 250);
      //
      
      TrainingData sample1(1); 
      // sample1.addLabel(-1);
      // sample1.addFeature(35, 1.0);
      
      std::vector<Key> keys{0};
      std::vector<TrainingData> vals{sample1};
      //
      LOG(INFO) << "Add Data to kDataTableId : <" << keys[0] << ":" << vals[0];
      datatable.Add(keys, vals);
      LOG(INFO) << "Clock to kDataTableId ";
      datatable.Clock();
    });
    LOG(INFO) << "Start Loading Data Task";
    engine.Run(loadDataTask);
    
    MLTask task;
    task.SetWorkerAlloc({{0, 3}});  // 3 workers on node 0
    task.SetTables({kTableId});     // Use table 0
    task.SetLambda([kTableId, kDataTableId](const Info& info) {
      LOG(INFO) << "Hi";
      LOG(INFO) << info.DebugString();
      // LOG(INFO) << info.partition_manager_map.find(kTableId) ;
      auto datatable = info.CreateKVClientTable<TrainingData>(kDataTableId);
      std::vector<Key> keys{0};
      std::vector<TrainingData> ret;
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
