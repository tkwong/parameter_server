#pragma once
#include "lda_doc.hpp"
#include <vector>
#include <list>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <random>
#include <unordered_map>

namespace LDA{
	// struct TopicWordCount{
	// 	int32_t topic_idx;
	// 	int32_t word_count;
	// };

	class SparseSampler{
	public:
		//SparseSampler();
		SparseSampler(int32_t _V, const std::vector<int>& _topic_summary_table, const std::vector<std::unordered_map<int32_t, int32_t>>& _word_topic_table);
		~SparseSampler();
		

		void sampleDoc(LDADoc& doc);

		int32_t sampleTopic();

		void computeDocTopic(LDADoc doc);

		void initializePrecomputableVariable();



	private:
		int K;
		int V;
		double beta;
		double beta_sum;
		double alpha;

		std::vector<int32_t> topic_summary_table;			//	number of words assigned to topics i, row vector of size K (nTopics)

			
		std::vector<std::unordered_map<int32_t, int32_t>> word_topic_table;	//	number of vocab j assigned to topic j, size V x K (nVocabs x nTopics)
		std::vector<int32_t> doc_topic_vector;	//	number of words in document i assigned to topic j, size M x K (nDocs x nTopics)s
		// std::vector<std::vector<int32_t>> doc_topic_vector;	//	number of words in document i assigned to topic j, size M x K (nDocs x nTopics)s
		double s_sum;									//	s term
		double r_sum;									//	r term
		double q_sum;									//	q term
		std::vector<double> q_coeff;		// 
		//std::vector<double> nonzero_q_terms;
  		//std::vector<double> nonzero_q_terms_topic;

  		struct QTermTopic{
  			double nonzero_q_term;
  			int32_t topic;
  		};
  		int32_t num_nonzero_q_terms;
  		std::list<QTermTopic> nonzero_q_terms_topic;
  		/*
  		int32_t* nonzero_doc_topic_idx_;
		int32_t num_nonzero_doc_topic_idx_;
		*/
//		std::list<int32_t> nonzero_doc_topic_idx;
		int32_t* nonzero_doc_topic_idx;
		int32_t num_nonzero_doc_topic_idx;
		//std::unique_ptr<std::mt19937> rng_engine;




	};
}
