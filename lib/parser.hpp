#pragma once

#include <cstring>
#include "boost/utility/string_ref.hpp"

namespace csci5570 {
namespace lib {

template <typename Sample, typename DataStore>
class Parser {
 public:
  /**
   * Parsing logic for one line in file
   *
   * @param line    a line read from the input file
   */
  static Sample parse_libsvm(boost::string_ref line, int n_features) {
    // check the LibSVM format and complete the parsing
    // hints: you may use boost::tokenizer, std::strtok_r, std::stringstream or any method you like
    // so far we tried all the tree and found std::strtok_r is fastest :)
    
    Sample sample;
    DataStore ds;
	
	// strtok_r and strtok operations :  see https://stackoverflow.com/questions/15961253/c-correct-usage-of-strtok-r
	char *str = strdup(line.data());
    char *saveptr1 = NULL;
    
    char *token = strtok_r(str, " ", &saveptr1);
    // sample.x_ = token; //FIXME: Sample does not have x_
    
    for (int i=0; i < n_features; i++) // parse n_features only.
    {
        char *val = NULL;
        char *key = strtok_r(str, ":", &val);
        
        // ds.insert(key,val);
    }

    // sample.y_ = ds;//FIXME: sample does not have y_
    
    return sample;
  }

  static Sample parse_mnist(boost::string_ref line, int n_features) {
    // check the MNIST format and complete the parsing
  }

  // You may implement other parsing logic

};  // class Parser

}  // namespace lib
}  // namespace csci5570
