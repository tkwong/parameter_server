#pragma once

#include <functional>
#include <string>

#include "boost/utility/string_ref.hpp"

#include "lib/parser.hpp"
#include "lib/LUrlParser/LUrlParser.hpp"
#include "io/coordinator.hpp"
#include "io/line_input_format.hpp"

namespace csci5570 {
namespace lib {

// template <typename Sample, template <typename> typename DataStore<Sample>>
template <typename Sample, typename DataStore>
class AbstractDataLoader {
 public:
  /**
   * Load samples from the url into datastore
   *
   * @param url          input file/directory
   * @param n_features   the number of features in the dataset
   * @param parse        a parsing function
   * @param datastore    a container for the samples / external in-memory storage abstraction
   */
  template <typename Parse>  // e.g. std::function<Sample(boost::string_ref, int)>
  static void load(std::string url, int n_features, Parse parse, DataStore* datastore,
        int second_id, int num_threads, std::string master_host, int master_port ,std::string worker_host) {
    // 1. Connect to the data source, e.g. HDFS, via the modules in io
    LUrlParser::clParseURL URL = LUrlParser::clParseURL::ParseURL(url);
    std::string hdfs_namenode = URL.m_Host;
    int hdfs_namenode_port = stoi(URL.m_Port);
    // int master_port = 19818;  // use a random port number to avoid collision with other users
    zmq::context_t zmq_context(1);
    
    int proc_id = getpid();
    // std::string master_host = "proj10";
    //std::string worker_host = "proj10";
    
    Coordinator coordinator(proc_id, worker_host, &zmq_context, master_host, master_port);
    coordinator.serve();
    LOG(INFO) << "Coordinator begins serving";
    
    //int num_threads = 1;
    //int second_id = 0;
    LineInputFormat infmt("/"+URL.m_Path, num_threads, second_id, &coordinator,
        worker_host, hdfs_namenode, hdfs_namenode_port);
    LOG(INFO) << "Line input is well prepared";
    
    // 2. Extract and parse lines
    // 3. Put samples into datastore
    bool success = true;
    boost::string_ref record;
    while (true) {
        success = infmt.next(record);
        if (success == false)
            break;
        Sample sample = parse(record, n_features);
        datastore->push_back(sample);
    }
    
    BinStream finish_signal;
    finish_signal << worker_host << second_id;
    coordinator.notify_master(finish_signal, 300);
  }

};  // class AbstractDataLoader

}  // namespace lib
}  // namespace csci5570
