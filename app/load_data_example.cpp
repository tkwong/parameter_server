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
    
    Node my_node{0, "localhost", 12353};
    std::vector<Node> nodes{{0, "localhost", 12353}};
    
    Engine engine(my_node, nodes);
    
    // start
    engine.StartEverything();
    const auto kTableId = engine.CreateTable<double>(ModelType::SSP, StorageType::Map);  // table 0
    const auto kDataTableId = engine.CreateTable<Sample>(ModelType::BSP, StorageType::Map);  // table 1
    
    engine.Barrier();
    
    // MLTask loadDataTask;
    // loadDataTask.SetWorkerAlloc({{0,3}});
    // loadDataTask.SetTables({kDataTableId});
    // loadDataTask.SetLambda([kDataTableId](const Info& info) {
    //   LOG(INFO) << "DataLoadingTask : I am " << info.DebugString();
    //
    //   auto datatable = info.CreateKVClientTable<Sample>(kDataTableId);
    //
    //   // FIXME: Parse the sample data, now using random data.
    //
    //   std::vector<boost::string_ref> lines ({
    //     "1 99:1.1 207:1.2 208:1.3 225:1.4",
    //     "-1 99:1.1 207:1.2 208:1.3 225:1.4",
    //     "-1 99:1.1 207:1.2 208:1.3 225:1.4",
    //     "-1 99:1.1 207:1.2 208:1.3 225:1.4"
    //   });
    //
    //   for(auto line : lines) {
    //
    //     // Use WorkerID as the Key, now can distribute over nodes. like round robin or sample or weighted
    //     std::vector<Key> keys{info.worker_id};
    //
    //     // Get first. if not clocked by ML wzorkers, it will wait.
    //     std::vector<Sample> ret;
    //     LOG(INFO) << "Data Get : "<< keys[0];
    //     datatable.Get(keys, &ret);
    //     for (auto r_ : ret )
    //       LOG(INFO) << "Data GOT Sample : "<< keys[0] << " : " << r_;
    //
    //     Sample sample(1.1);
    //     // Sample sample(line); // FIXME: Now only can hard code before actually can compile <std::string> Sample
    //     // Use Sample as Value
    //     std::vector<Sample> vals{sample};
    //     //
    //     LOG(INFO) << "Adding Data to kDataTableId : " << keys[0] ;
    //     datatable.Add(keys, vals);
    //
    //     LOG(INFO) << "Clock to kDataTableId ";
    //     datatable.Clock();
    //   }
    //
    // });
    // LOG(INFO) << "Start Loading Data Task";
    // engine.Run(loadDataTask);
    
    MLTask task;
    task.SetWorkerAlloc({{0, 2}});  // 2 workers on node 0
    task.SetTables({kTableId, kDataTableId});     // Use table 0,1
    task.SetLambda([kTableId, kDataTableId](const Info& info) {
      
      if (info.worker_id  == 0){ // worker_id 0 is for data loading (1 worker only? )
        
        LOG(INFO) << "DataLoadingTask : I am " << info.DebugString();
      
        auto datatable = info.CreateKVClientTable<Sample>(kDataTableId);
      
        // FIXME: Parse the sample data, now using random data.
      
        std::vector<boost::string_ref> lines ({
          "1 99:1.1 207:1.2 208:1.3 225:1.4",
          "-1 99:1.1 207:1.2 208:1.3 225:1.4",
          "-1 99:1.1 207:1.2 208:1.3 225:1.4",
          "-1 99:1.1 207:1.2 208:1.3 225:1.4"
        });
      
        int data_loop_count = 1; 
        for(auto line : lines) {

          // Use WorkerID as the Key, now can distribute over nodes. like round robin or sample or weighted
          // FIXME: For Testing, only assign to worker 1
          std::vector<Key> keys{1};

          std::vector<Sample> ret;
          datatable.Get(keys, &ret);
          
          Sample sample(data_loop_count);
          // Sample sample(line); // FIXME: Now only can hard code before actually can compile <std::string> Sample
          // Use Sample as Value
          std::vector<Sample> vals{sample};
          //
          LOG(INFO) << "Adding Data ["<< data_loop_count << "]: " << keys[0] << ":" << data_loop_count;
          datatable.Add(keys, vals);
        
          LOG(INFO) << "Clock to ["<< data_loop_count << "] kDataTableId ";
          datatable.Clock();
          
          ++data_loop_count;
        }
        // HACK. When finished loading, Add and Clock a magic number to stop.
        double special_val(-1);
        
        std::vector<Key> keys{1};
        std::vector<Sample> vals{special_val};
        
        LOG(INFO) << "Finished Loading, issuing Add " << special_val ; 
        datatable.Add(keys, vals);
        datatable.Clock();
         
      } else {
        
        LOG(INFO) << "MLTask : I am " << info.DebugString();
        auto datatable = info.CreateKVClientTable<Sample>(kDataTableId);
        
        std::vector<Key> keys{info.worker_id};
        std::vector<Sample> ret;

        datatable.Clock();  // First clock to load first data into model
        LOG(INFO) << "First Clock to kDataTableId ";        

        int worker_loop_count = 0;
        while(true) { // loop the data samples
          
          LOG(INFO) << "Worker Get ["<< worker_loop_count << "] : "<< keys[0];
          datatable.Get(keys, &ret);
          for (auto r_ : ret) {
            LOG(INFO) << "Worker Get Reply ["<< worker_loop_count << "] : "<< keys[0] << " : " << r_;
          }
          
          // HACK. special character for quit
          if(ret[0] == -1){
            LOG(INFO) << "Received special vals, Stop ML Task"; 
            break;
          }
          // FIXME: This Clock seems not able to clock the data loading task
          LOG(INFO) << "Worker Clock to ["<< worker_loop_count << "] kDataTableId ";
          datatable.Clock();

          // KVClientTable<double> table = info.CreateKVClientTable<double>(kTableId);
          // for (int i = 0; i < 5; ++i) {
          //   std::vector<Key> keys{1};
          //   std::vector<double> vals{0.5};
          //   table.Add(keys, vals);
          //   std::vector<double> ret;
          //   table.Get(keys, &ret);
          //   LOG(INFO) << ret[0];
          // }
          ++worker_loop_count;
        }
        
      }
      
    });
    LOG(INFO) << "Start ML Task";
    engine.Run(task);

    // stop
    engine.StopEverything();
    return 0;
}
