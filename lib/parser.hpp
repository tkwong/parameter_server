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
	char *token;
	char *rest = strdup(line.data());

    for (int i=0; i < n_features; i++) // parse n_features only.
    {
		token = strtok_r(rest, ",", &rest);
		if (i==0){ // FIXME: 
			// sample.x_ = token; //FIXME: Sample does not have x_
		} else {
			// auto key = std::strtok(token, ":");
			// auto val = std::strtok(token, " ");
			// ds.insert(key,val);
		}
    }

    // sample.y_ = ds;//FIXME: sample does not have y_
    
    return sample;
  }

  static Sample parse_mnist(boost::string_ref line, int n_features) {
    // check the MNIST format and complete the parsing
    int MNISTDataSet::LABEL_MAGIC = 2049;  // the magic number given in the dataset
    int MNISTDataSet::IMAGE_MAGIC = 2051;

    int ReverseInt(int i)  // the integers are stored in MSB first manner
    {
      unsigned char ch1, ch2, ch3, ch4;
      ch1 = i & 255;
      ch2 = (i >> 8) & 255;
      ch3 = (i >> 16) & 255;
      ch4 = (i >> 24) & 255;
      return ((int)ch1 << 24) + ((int)ch2 << 16) + ((int)ch3 << 8) + ch4;
    }
 
    for (int i=0; i < n_features; i++) // parse n_features only.
    {
      token = strtok_r(rest, ",", &rest);
    if (i==0){ // FIXME: 
       sample.x_ = token; //FIXME: Sample does not have x_

    } else {
       auto key = std::strtok(token, ":");
       auto val = std::strtok(token, " ");
       ds.insert(key,val);
    }
    }


  }

  // You may implement other parsing logic

};  // class Parser

}  // namespace lib
}  // namespace csci5570
