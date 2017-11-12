#include "base/message.hpp"

#include "server/abstract_storage.hpp"

#include "glog/logging.h"

#include <vector>

namespace csci5570{

	template <typename Val>
	class VectorStorage : public AbstractStorage {
		public:
		VectorStorage() = default;

		virtual void SubAdd(const third_party::SArray<Key>& typed_keys, 
			const third_party::SArray<char>& vals) override {

			auto typed_vals = third_party::SArray<Val>(vals);//transfer the value from char
			CHECK_EQ(typed_keys.size(), typed_vals.size());
			for (int i=0; i<typed_keys.size(); i++){
	             storage_keys.insert(storage_keys.end(), typed_keys[i]);
	             //LOG(INFO) << "hello";
	             //LOG(INFO) << storage_keys[0];
	             storage_vals.insert(storage_vals.end(), typed_vals[i]);
               // LOG(INFO) << "vals here";
               // LOG(INFO) << storage_vals[i];
               // LOG(INFO) << typed_vals[i];
			}
			
		} 
		virtual third_party::SArray<char> SubGet(const third_party::SArray<Key>& typed_keys) override{
			third_party::SArray<Val> reply_vals(typed_keys.size());
     		for (int i=0; i<storage_keys.size(); i++) {
     			for (int m=0; m<typed_keys.size(); m++){
     				if (storage_keys[i] == typed_keys[m]){
		        		reply_vals[m] = storage_vals[i];
		        	    //LOG(INFO) << "vals here output";
	                 	//LOG(INFO) << reply_vals[m];

	                 }
            else
              break; //If a key does not exist, return the values founded so far.
     			}
	    	}
	    	LOG(INFO) << "berfore return";

	    	LOG(INFO) << reply_vals;

	    	return third_party::SArray<char>(reply_vals);
  		}

  		virtual void FinishIter() override {}

	  	private:
		std::vector<int> storage_keys;
		std::vector<Val> storage_vals;
	};
}// namespace csci5570 
