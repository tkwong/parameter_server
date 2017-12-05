#include "gflags/gflags.h"
#include "glog/logging.h"

#include <gperftools/profiler.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "boost/utility/string_ref.hpp"
#include "boost/tokenizer.hpp"
#include "boost/lexical_cast.hpp"

#include "base/serialization.hpp"

#include "driver/engine.hpp"

#include "io/hdfs_manager.hpp"

#include "worker/kv_client_table.hpp"

#include "lda_doc.hpp"
#include "lda_stat.hpp"
#include "doc_sampler.hpp"

DEFINE_int32(my_id, -1, "The process id of this program");
DEFINE_string(config_file, "", "The config file path");
DEFINE_string(hdfs_namenode, "", "The hdfs namenode hostname");
DEFINE_string(input, "", "The hdfs input url");
DEFINE_int32(hdfs_namenode_port, -1, "The hdfs namenode port");
DEFINE_int32(kSpeculation, 1, "speculation");
DEFINE_string(kSparseSSPRecorderType, "", "None/Map/Vector");
DEFINE_int32(num_servers_per_node, 1, "num_servers_per_node");
DEFINE_int32(num_workers_per_node, 1, "num_workers_per_node");
DEFINE_int32(num_iters, 100, "number of iters");
DEFINE_int32(kStaleness, 0, "stalness");
DEFINE_string(kModelType, "", "ASP/SSP/BSP/SparseSSP");
DEFINE_string(kStorageType, "", "Map/Vector");
DEFINE_int32(num_dims, 0, "number of dimensions");
DEFINE_int32(batch_size, 100, "batch size of each epoch");
DEFINE_double(alpha, 0.1, "alpha");
DEFINE_double(beta, 0.1, "beta");
DEFINE_int32(num_topics, 10, "num_topics");
DEFINE_int32(max_voc_id, -1, "max_voc_id");
DEFINE_int32(num_docs, -1, "num_docs");
DEFINE_string(result_write_path, "result.txt", "result write path");
DEFINE_int32(num_epochs, 1, "number of epochs");
DEFINE_int32(compute_llh, 0, "compute log-likehood");
DEFINE_int32(num_process, 1, "number of processes");
DEFINE_int32(num_batches, 100, "number of batches");
DEFINE_int32(max_vocs_each_pull, 10000, "max vocs in each poll");


namespace csci5570 {
        // initiate Word-Topic Table and Topic Summary Table
        void init_tables(std::vector<csci5570::LDADoc>& corpus, Info info, int LDATableId, int num_topics, int max_voc_id) {
                // 0. Create key val map
                std::map<int, int> word_topic_count;
                int sum = 0;
                //LOG(INFO) << info.DebugString() << corpus.size();
                if (!corpus.empty()) {
                        int topic_summary_start =  max_voc_id * num_topics;
                        for (auto& doc : corpus) {
                                for (LDADoc::Iterator it(&doc); !it.IsEnd(); it.Next()) {
                                        int word = it.Word();
                                        int topic = it.Topic();
                                        // update word topic
                                        sum+=1;

                                        word_topic_count[word * num_topics + topic] += 1;
                                        word_topic_count[topic_summary_start + topic] += 1;
                                }
                        }
                }
                LOG(INFO)<< info.DebugString() << " sum is:"<<sum << "word_topic_count: " << word_topic_count.size();
                auto lda_table = info.CreateKVClientTable<float>(LDATableId);
                std::vector<Key> lda_key;
                std::vector<float> params;

                for (auto& key_val : word_topic_count) {
                        lda_key.push_back(key_val.first);
                }
                LOG(INFO) << info.DebugString() << " lda_key.size(): " << lda_key.size();
                lda_table.Get(lda_key, &params);
                LOG(INFO) << "word_topic_count.size(): " << word_topic_count.size() << " lda_key.size(): " << lda_key.size() << " params.size(): " << params.size();
                int i = 0;
                for (auto& key_val : word_topic_count) {
                        params[i] = key_val.second;
                        i++;
                }
                lda_table.Add(lda_key, params);
        }

        //void batch_training_by_chunk(std::vector<csci5570::LDADoc>& corpus, int LDATableId, int num_total_workers, int num_topics, int max_voc_id, int num_iterations,
        void batch_training_by_chunk(std::vector<csci5570::LDADoc>& corpus, KVClientTable<float>& lda_table, KVClientTable<float>& lda_stat_table, int num_total_workers, int num_topics, int max_voc_id, int num_iterations,
                        float alpha, float beta, const Info& info ) {
                // 1. Find local vocabs and only Push and Pull those
                // 2. Bosen parameters synchronization:
                //    1. every iteration(called work unit) go through the corpus num_iters_per_work_unit times.
                //    2. every work unit will call Clock() num_clocks_per_work_unit times.
                //
                //    Our current synchronization:
                //    Same as setting num_iters_per_work_unit = num_clocks_per_work_unit.

                /*  difine the constants used int this function*/
                int need_compute_llh = FLAGS_compute_llh;
                int num_epochs = FLAGS_num_epochs;
                int num_process = FLAGS_num_process;
                int num_batches = FLAGS_num_batches;
                int max_vocs_each_pull = FLAGS_max_vocs_each_pull;
                std::string result_write_path = FLAGS_result_write_path;
                int num_workers = num_total_workers;

                int num_vocs_per_thread = max_voc_id/num_workers;
                int vocab_id_start =  info.worker_id * num_vocs_per_thread;
                int vocab_id_end = (info.worker_id + 1 == num_workers) ? max_voc_id : vocab_id_start + num_vocs_per_thread;

                std::chrono::duration<double> pull_time;
                std::chrono::duration<double> push_time;
                std::chrono::duration<double> sample_time;
                std::chrono::duration<double> train_time;

                std::chrono::duration<double> llh_push_time;
                std::chrono::duration<double> llh_pull_time;
                std::chrono::duration<double> mailbox_time;
                std::chrono::duration<double> llh_time;

                int lda_llh_idx = 0;
                int pull_time_idx = 1;
                int push_time_idx = 2;
                int sample_time_idx = 3;
                int train_time_idx = 4;

                int llh_pull_time_idx  = 5;
                int llh_push_time_idx = 6;
                int mailbox_time_idx = 7;
                int llh_time_idx = 8;

                std::vector<csci5570::Key> stat_keys(9);
                std::iota(stat_keys.begin(), stat_keys.end(), 0);
                std::vector<float> llh_and_time_updates(9, 0.0);
                std::vector<float> llh_and_time(9, 0.0);

                //auto* kvworker = kvstore::KVStore::Get().get_kvworker(info.get_local_id());
                //auto lda_table = info.CreateKVClientTable<float>(LDATableId);
                //---------------------------------------------
                //For multiple epoch task only (SPMT for example)
                //bool is_last_epoch = (Context::get_param("kType") == "PS") ? true : ((info.get_current_epoch() + 1) % num_process == 0);

                //std::vector<std::vector<float>> word_topic_count;
                //std::vector<std::vector<float>> word_topic_updates;
                std::vector<float> word_topic_count;
                std::vector<float> word_topic_updates;
                //std::map<int, int> find_start_idx; // map the word to the start index of in batch_voc_keys and word_topic_count
                std::map<int, int> find_start_word_topic_idx; // map the word to the start index of in batch_voc_topic_keys and word_topic_count

                for (int i=1; i<=num_iterations; i++) {
                        LOG(INFO) << info.DebugString() << " Iteration: " << i;
                        auto start_iter_time = std::chrono::steady_clock::now();

                        //  Divide the corpus into batches to do batch update
                        // [j * num_docs_per_batch, j * num_docs_per_batch + num_docs_per_batch) for first num_batches-1
                        // [(num_batches - 1) * num_docs_per_batch, corpus.size()) for last batch
                        assert(num_batches > 0);
                        int num_docs_per_batch = corpus.size() / num_batches;
                        std::vector<int> batch_end(num_batches);
                        std::vector<int> batch_begin(num_batches);
                        for (int j = 0; j < num_batches; j ++) {
                                batch_begin[j] = j * num_docs_per_batch;
                                batch_end[j]  = (j + 1 == num_batches) ? corpus.size() : batch_begin[j] +  num_docs_per_batch;
                        }

                        int num_keys_pulled = 0;
                        for (int j = 0; j<num_batches; j++) {
                                if (info.worker_id % FLAGS_num_workers_per_node == 0 && j % FLAGS_num_workers_per_node == 0)
                                        LOG(INFO) << info.DebugString() << " current batches: " << j;
                                //-----------------------------------
                                // 1. make keys for local vocs needed for sampling
                                std::set<int> batch_voc_set;
                                for (int k = batch_begin[j]; k < batch_end[j]; k++) {
                                        for (LDADoc::Iterator it(&(corpus[k])); !it.IsEnd(); it.Next()) {
                                                // batch_voc_set will be sorted in ascending order
                                                batch_voc_set.insert(it.Word());
                                        }
                                }

                                // --------------------------------------------
                                // Further reduce the number of keys for each batch
                                // LightLDA. It is like coordinate descent.
                                auto iter = batch_voc_set.begin();
                                int debug_count = 0; // for debugging...
                                while (iter != batch_voc_set.end()) {
                                        //if (debug_count % 100 == 0)
                                        //LOG(INFO) << info.DebugString() << " debug_count: " << debug_count++;
                                        std::vector<Key> batch_voc_keys;
                                        batch_voc_keys.reserve((batch_voc_set.size()+1));
                                        //find_start_idx.clear();
                                        find_start_word_topic_idx.clear();

                                        //-------------------------------------------------------
                                        // Make keys for word_topic_table and topic_summary row.
                                        // Each word corresponds to one chunk.
                                        auto key_1d_gen_start_time = std::chrono::steady_clock::now();
                                        for (int count = 0; count < max_vocs_each_pull && iter != batch_voc_set.end(); count ++) {
                                                // Put all topics count for this word in batch_voc_keys[count * num_topics, (count + 1) * num_topics)
                                                batch_voc_keys.push_back(*iter);
                                                iter++;
                                        }
                                        // ----------------------------------------------------------------
                                        // (optional) add those keys needed for computing llh.
                                        if (need_compute_llh) {
                                                // reserve more space
                                                batch_voc_keys.reserve((batch_voc_set.size() + num_vocs_per_thread + 1));
                                                for (int m = vocab_id_start; m < vocab_id_end; m++) {
                                                        batch_voc_keys.push_back(m);
                                                }
                                                // remove duplicates
                                                std::sort(batch_voc_keys.begin(), batch_voc_keys.end());
                                                batch_voc_keys.erase(std::unique(batch_voc_keys.begin(), batch_voc_keys.end()), batch_voc_keys.end());
                                        }
                                        auto key_1d_gen_end_time = std::chrono::steady_clock::now();
                                        std::chrono::duration<double> key_1d_gen_duration = key_1d_gen_end_time - key_1d_gen_start_time;
                                        if (info.worker_id % FLAGS_num_workers_per_node == 0 && j % FLAGS_num_workers_per_node == 0)
                                                LOG(INFO) << " gen 1d key time: " << key_1d_gen_duration.count();

                                        // 1-D voc-topic key
                                        // [word topic table, topic summary table]
                                        // [(w1,t1),(w1,t2),...,(w1,tk),(w2,t1),(w2,t2),...,(w2,tk),.......,(wv,tk),(t1),(t2),...,(tk)]]
                                        // [...(wi * ntopics + k)...(t1),(t2),...,(tK)]
                                        std::vector<Key> batch_voc_topic_keys;
                                        batch_voc_topic_keys.reserve(batch_voc_keys.size() * num_topics);
                                        auto key_2d_gen_start_time = std::chrono::steady_clock::now();
                                        for (int ii = 0; ii < batch_voc_keys.size(); ii++)
                                        {
                                                for (int jj = 0; jj < num_topics; jj++)
                                                        batch_voc_topic_keys.push_back(batch_voc_keys[ii] * num_topics + jj);
                                        }
                                        auto key_2d_gen_end_time = std::chrono::steady_clock::now();
                                        std::chrono::duration<double> key_2d_gen_duration = key_2d_gen_end_time - key_2d_gen_start_time;
                                        if (info.worker_id % FLAGS_num_workers_per_node == 0 && j % FLAGS_num_workers_per_node == 0)
                                                LOG(INFO) << " gen 2d key time: " << key_2d_gen_duration.count();

                                        // calculate the key map
                                        /*for (int count = 0; count < batch_voc_keys.size(); count++) {
                                          find_start_idx[batch_voc_keys[count]] = count;
                                          }*/

                                        // for 1-D mapping
                                        // [(w1 * ntopics, 0), (w2 * ntopics, 1),...(wn * num_topics, n)]
                                        for (int count = 0; count < batch_voc_keys.size(); count++) {
                                                //find_start_word_topic_idx[batch_voc_keys[count] * num_topics] = count;
                                                find_start_word_topic_idx[batch_voc_keys[count]] = count * num_topics;
                                        }

                                        //LOG(INFO) << info.DebugString() << " Before: " << "batch_voc_topic_keys.size(): " << batch_voc_topic_keys.size() << "batch_voc_keys.size(): " << batch_voc_keys.size() << "first_start_idx.size(): "<< find_start_idx.size() << "first_start_idx.size(): "<< find_start_idx.size();
                                        batch_voc_keys.push_back(max_voc_id);
                                        //find_start_idx[max_voc_id] = batch_voc_keys.size() - 1;
                                        //num_keys_pulled = batch_voc_keys.size() * num_topics;

                                        for (int ii = 0; ii < num_topics; ii++)
                                                batch_voc_topic_keys.push_back(max_voc_id * num_topics + ii);
                                        //find_start_word_topic_idx[max_voc_id * num_topics] = batch_voc_topic_keys.size() - num_topics;
                                        find_start_word_topic_idx[max_voc_id] = max_voc_id * num_topics;
                                        num_keys_pulled = batch_voc_topic_keys.size();

                                        //LOG(INFO) << info.DebugString() << " After: " << "batch_voc_topic_keys.size(): " << batch_voc_topic_keys.size() << "batch_voc_keys.size(): " << batch_voc_keys.size() << "first_start_idx.size(): "<< find_start_idx.size() << "first_start_idx.size(): "<< find_start_idx.size();

                                        // -----------------------------------------
                                        // clear previous count and updates
                                        //word_topic_count = std::move(std::vector<std::vector<float>>(batch_voc_keys.size(), std::vector<float>(num_topics, 0)));
                                        //word_topic_updates = std::move(std::vector<std::vector<float>>(batch_voc_keys.size(), std::vector<float>(num_topics, 0)));
                                        //word_topic_count.clear();
                                        //word_topic_updates.clear();
                                        word_topic_count = std::move(std::vector<float>(batch_voc_keys.size() * num_topics, 0));
                                        word_topic_updates = std::move(std::vector<float>(batch_voc_keys.size() * num_topics, 0));
                                        word_topic_count.clear();
                                        word_topic_updates.clear();
                                        //std::fill(word_topic_count.begin(), word_topic_count.end(), 0);
                                        //std::fill(word_topic_updates.begin(), word_topic_updates.end(), 0);
                                        {
                                                // Pull chunks from kvstore
                                                std::vector<std::vector<float>*> word_topic_count_ptrs(batch_voc_keys.size());
                                                //LOG(INFO) << info.DebugString() << " word_topic_count.size(): " << word_topic_count.size();
                                                for ( int i=0; i<word_topic_count.size(); i++) {
                                                        //word_topic_count_ptrs[i] = &(word_topic_count[i]);
                                                }

                                                auto start_pull_time = std::chrono::steady_clock::now();
                                                //LOG(INFO) << info.DebugString() << " Before Get(): " << "batch_voc_topic_keys.size()" << batch_voc_topic_keys.size() << " word_topic_count.size: " << word_topic_count.size();
                                                lda_table.Get(batch_voc_topic_keys, &word_topic_count);
                                                //mlworker->PullChunks(batch_voc_keys, word_topic_count_ptrs);
                                                auto end_pull_time = std::chrono::steady_clock::now();
                                                pull_time = end_pull_time - start_pull_time ;
                                                if (info.worker_id % FLAGS_num_workers_per_node == 0 && j % FLAGS_num_workers_per_node == 0)
                                                        LOG(INFO) << info.DebugString() << " iter: " << i << " Total Pull Time = " << pull_time.count() << " Key size: " << batch_voc_topic_keys.size();

                                                //LOG_I<<"iter:" << i << " thread id:" <<info.get_global_id()<< " time taken:" <<pull_time.count()<<" Key size:"<<batch_voc_keys.size();
                                        }
                                        word_topic_updates.resize(word_topic_count.size());
                                        ////--------------------------------------------------------
                                        //// Doc sampler is responsible for
                                        //// 1. Compute the updates stored it in word_topic_updates;
                                        //// 2. Sample new topics for each word in corpus
                                        //LOG(INFO) << "Start sampling";
                                        auto start_sample_time = std::chrono::steady_clock::now();
                                        DocSampler doc_sampler(num_topics, max_voc_id, alpha, beta);
                                        for (int k = batch_begin[j]; k < batch_end[j]; k++) {
                                                //doc_sampler.sample_one_doc(corpus[k], word_topic_count, word_topic_updates, find_start_idx);
                                                //LOG(INFO) << " Sampling Doc: [" << k << "]";
                                                doc_sampler.sample_one_doc(corpus[k], word_topic_count, word_topic_updates, find_start_word_topic_idx);
                                        }
                                        auto end_sample_time = std::chrono::steady_clock::now();

                                        sample_time = end_sample_time - start_sample_time;
                                        if (info.worker_id % FLAGS_num_workers_per_node == 0 && j % FLAGS_num_workers_per_node == 0)
                                                LOG(INFO) << " sampling time: " << sample_time.count();
                                        {
                                                // Push the updates to kvstore.
                                                std::vector<std::vector<float>*> word_topic_updates_ptrs(batch_voc_keys.size());
                                                for ( int i=0; i<word_topic_updates.size(); i++) {
                                                        //###word_topic_updates_ptrs[i] = &(word_topic_updates[i]);
                                                }
                                                auto start_push_time = std::chrono::steady_clock::now();
                                                //lda_table.Add(batch_voc_keys, word_topic_updates_ptrs);
                                                lda_table.Add(batch_voc_topic_keys, word_topic_updates);
                                                lda_table.Clock();
                                                //mlworker->PushChunks(batch_voc_keys, word_topic_updates_ptrs);
                                                auto end_push_time = std::chrono::steady_clock::now();
                                                push_time =  end_push_time - start_push_time;
                                                if (info.worker_id % FLAGS_num_workers_per_node == 0 && j % FLAGS_num_workers_per_node == 0)
                                                        LOG(INFO) << info.DebugString() << " iter: " << i << " Total Push Time = " << push_time.count() << " Key size: " << batch_voc_topic_keys.size();
                                        }

                                }
                        } // end of batch iter
                        auto end_train_time = std::chrono::steady_clock::now();
                        train_time = end_train_time - start_iter_time;

                        //LOG_I<<"iter:"<<i<<" client:"<<info.get_cluster_id()<< " training time:"<<train_time.count();

                        if (need_compute_llh) {
                                std::vector<int> topic_summary(num_topics);
                                int topic_summary_start = find_start_word_topic_idx[max_voc_id * num_topics];
                                int sum = 0;
                                for (int k = 0; k < num_topics; k++) {
                                        topic_summary[k] = word_topic_count[topic_summary_start + k];
                                        sum += topic_summary[k];
                                        if (info.worker_id == 0) {
                                                LOG(INFO)<<topic_summary[k]<<" ";
                                        }
                                }
                                if (info.worker_id == 0) {
                                        LOG(INFO)<<" sum is:"<<sum<<std::endl;
                                }

                                LDAStats lda_stats(topic_summary, num_topics, max_voc_id, alpha, beta);

                                double sum_llh = 0;
                                // -------------------------------------------------------------
                                // 1. Word llh. Only computed in the last iteration of last epoch
                                //if ((i == num_iterations && is_last_epoch) || (Context::get_param("kType") == "PS")) {
                                sum_llh += lda_stats.ComputeWordLLH(vocab_id_start, vocab_id_end, word_topic_count, find_start_word_topic_idx);
                                //}
                                double word_llh = sum_llh;

                                // ---------------------------------------------------------------
                                // 2. Doc llh. Each thread computes by going through local corpus.
                                // Computed in the last iteration of every epoch
                                //if (i == num_iterations || Context::get_param("kType") == "PS") {
                                for (auto& doc : corpus) {
                                        sum_llh += lda_stats.ComputeOneDocLLH(doc);
                                }
                                //}
                                double doc_llh = sum_llh - word_llh;

                                // 3. Word summary llh. Only one thread computes it at the end of all epochs for SPMT
                                // But is computed in every epoch and every iteration for PS
                                if (info.worker_id == 0) {
                                        sum_llh += lda_stats.ComputeWordLLHSummary();
                                }
                                double summary_llh = sum_llh - doc_llh - word_llh;

                                auto mailbox_start_time = std::chrono::steady_clock::now();

                                /*
                                   auto* mailbox = Context::get_mailbox(info.get_local_id());
                                   husky::base::BinStream bin;
                                   bin << sum_llh;
                                   int progress_id = i + num_iterations * info.get_current_epoch();
                                   mailbox->send(info.get_tid(0), 1, progress_id, bin);
                                   mailbox->send_complete(1, progress_id,
                                   info.get_worker_info().get_local_tids(), info.get_worker_info().get_pids());

                                   if (info.worker_id() == 0) {
                                   double agg_llh = 0;
                                   while (mailbox->poll(1, progress_id)) {
                                   auto bin = mailbox->recv(1, progress_id);
                                   double tmp = 0;
                                   bin >> tmp;
                                   if (!std::isnan(tmp)) {
                                   agg_llh += tmp;
                                   }
                                   }
                                // -----------------------------------------
                                // For SPMT task,
                                // only the last iteration sum all word llhs
                                {
                                LOG_I<<"local word llh:"<<word_llh<<" local doc_llh:"<<doc_llh<<" summary_llh:"<<summary_llh<<" agg_llh:"<<agg_llh;
                                llh_and_time_updates[lda_llh_idx] = agg_llh;
                                }
                                }
                                 */
                                auto mailbox_end_time = std::chrono::steady_clock::now();
                                mailbox_time = mailbox_end_time - mailbox_start_time;

                        } // end of compute llh
                        auto end_iter_time = std::chrono::steady_clock::now();
                        if (info.worker_id % FLAGS_num_workers_per_node == 0)
                        {
                                std::chrono::duration<double> iter_duration = end_iter_time - start_iter_time;
                                LOG(INFO) << info.DebugString() << " iteration duration: " << iter_duration.count();
                        }

                        if (info.worker_id == 0) {
                                llh_time = end_iter_time - end_train_time;
                                LOG(INFO)<<num_keys_pulled<<"keys. Pull:"<<pull_time.count()<<"s Push:"<<push_time.count()<<"s Sampling:"<<sample_time.count()<<" Train(P+P+S):"<<train_time.count()<<"s " <<"llh_update: "<<llh_and_time_updates[lda_llh_idx];
                                LOG(INFO)<<"mail_box time:"<<mailbox_time.count()<<"s "<<"llh_total_time:"<<llh_time.count()<<"s";
                                llh_and_time_updates[train_time_idx] = train_time.count();
                                llh_and_time_updates[pull_time_idx] = pull_time.count();
                                llh_and_time_updates[push_time_idx] = push_time.count();
                                llh_and_time_updates[sample_time_idx] = sample_time.count();
                                llh_and_time_updates[llh_pull_time_idx] = llh_pull_time.count();
                                llh_and_time_updates[llh_push_time_idx] = llh_push_time.count();
                                llh_and_time_updates[mailbox_time_idx] = mailbox_time.count();
                                llh_and_time_updates[llh_time_idx] = llh_time.count();

                                LOG(INFO)<<"llh and time updates";
                                for (auto a : llh_and_time_updates) {
                                        LOG(INFO)<<a;
                                }
                                //int ts = kvworker->Push(lda_stat_table, stat_keys, llh_and_time_updates, true);
                                //kvworker->Wait(lda_stat_table, ts);
                                std::vector<float> params;
                                lda_stat_table.Get(stat_keys, &params);
                                int i = 0;
                                for (auto vals : llh_and_time_updates){
                                        params[i] = vals;
                                        i++;
                                }
                                lda_stat_table.Add(stat_keys, params);
                                lda_stat_table.Clock();
                        }

                        if (info.worker_id == 0) {
                                std::ofstream ofs;
                                ofs.open(result_write_path, std::ofstream::out | std::ofstream::app);
                                if (!ofs.is_open()) {
                                        LOG(INFO)<<"Error~ cannot open file";
                                }
                                std::fill(llh_and_time.begin(), llh_and_time.end(), 0);
                                //int ts = kvworker->Pull(lda_stat_table, stat_keys, &llh_and_time, true);
                                //kvworker->Wait(lda_stat_table, ts);
                                lda_stat_table.Get(stat_keys, &llh_and_time);
                                // -----------------------------------
                                // For PS task, each iteration is just each one iteration since all the docs are went through
                                // For SPMT task, each iteration is num_process epochs when all the docs are went through
                                int iteration = i;
                                ofs << iteration <<"   ";
                                //ofs << std::setw(4) << llh_and_time[pull_time_idx] <<"s "<< llh_and_time[push_time_idx] <<"s " << llh_and_time[sample_time_idx] << "s "
                                ofs << llh_and_time[train_time_idx] <<" "<< llh_and_time[lda_llh_idx]<<" ";
                                ofs << std::setw(4) << llh_and_time[mailbox_time_idx]<<"s "<<llh_and_time[llh_time_idx]<<"s ";
                                ofs << std::setw(4) << llh_and_time[lda_llh_idx] << "s\n";
                                //LOG_I << "epoch:"<<info.get_current_epoch()<<" ";

                                LOG(INFO)<<"Push:";
                                for (auto& a : llh_and_time) {
                                        a *= -1.0;
                                        LOG(INFO)<<a;
                                }
                                // clear the llh and start next iteration
                                //ts = kvworker->Push(lda_stat_table, stat_keys, llh_and_time, true);
                                //kvworker->Wait(lda_stat_table, ts);
                                lda_stat_table.Add(stat_keys, llh_and_time);
                                lda_stat_table.Clock();
                                ofs.close();
                        }
                }  // end of iteration
        }









        void Run() {
                CHECK_NE(FLAGS_my_id, -1);
                CHECK(!FLAGS_config_file.empty());
                VLOG(1) << FLAGS_my_id << " " << FLAGS_config_file;

                // 0. Parse config_file
                std::vector<Node> nodes = ParseFile(FLAGS_config_file);
                CHECK(CheckValidNodeIds(nodes));
                CHECK(CheckUniquePort(nodes));
                CHECK(CheckConsecutiveIds(nodes));
                Node my_node = GetNodeById(nodes, FLAGS_my_id);
                LOG(INFO) << my_node.DebugString();

                // 1. Load data
                std::vector<LDADoc> corpus;
                HDFSManager::Config config;
                config.input = FLAGS_input;
                config.worker_host = my_node.hostname;
                config.worker_port = my_node.port;
                config.master_port = 19717;
                config.master_host = nodes[0].hostname;
                config.hdfs_namenode = FLAGS_hdfs_namenode;
                config.hdfs_namenode_port = FLAGS_hdfs_namenode_port;
                config.num_local_load_thread = 2;
                zmq::context_t* zmq_context = new zmq::context_t(1);
                std::mutex corpusLock;
                auto load_data_start_time = std::chrono::steady_clock::now();
                //### not using HDFS
                bool bHDFS = false;
                if (!bHDFS)
                {
                        std::ifstream infile;
                        infile.open(FLAGS_input);

                        if (!infile)
                        {
                                LOG(INFO) << "Cannot Open Dataset" << std::endl;
                        }

                        int num_docs = FLAGS_num_docs;
                        int num_nodes = nodes.size();
                        int my_node_id = FLAGS_my_id;
                        int local_corpus_size = round((double)num_docs / num_nodes);
                        int local_corpus_begin = my_node_id * local_corpus_size;
                        int local_corpus_end = (my_node_id == nodes.size() - 1) ? (num_docs - 1) : (local_corpus_begin + local_corpus_size - 1);

                        infile.seekg(std::ios::beg);
                        for(int i=0; i < local_corpus_begin; ++i){
                                infile.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
                        }
                        int doc_count = 0;
                        LOG(INFO) << " my id: " << my_node_id << " begin: " << local_corpus_begin << " end: " << local_corpus_end;
                        for(int i=0; i < local_corpus_end-local_corpus_begin + 1; i++){
                                LDADoc new_doc;
                                std::string readline;
                                if (!std::getline(infile, readline))
                                        break;
                                // parse doc_id pair1 pair2 ...
                                boost::char_separator<char> sep(" \n");
                                boost::tokenizer<boost::char_separator<char>> tok(readline, sep);
                                boost::tokenizer<boost::char_separator<char>>::iterator iter = tok.begin();
                                // the first is doc id
                                int doc_id = boost::lexical_cast<int>(*iter);
                                if (i == 0 || i == local_corpus_size - 1)
                                        LOG(INFO) << " my id: " << my_node_id << " doc id: " << doc_id;
                                ++iter;

                                // parse w1:f1 w2:f2 w3:f3 ...
                                for (; iter != tok.end(); ++iter) {
                                        std::string word_fre_pair = boost::lexical_cast<std::string>(*iter);
                                        boost::char_separator<char> sep(":");
                                        boost::tokenizer<boost::char_separator<char>> tok2(word_fre_pair, sep);
                                        boost::tokenizer<boost::char_separator<char>>::iterator iter1, iter2 = tok2.begin();
                                        iter1 = iter2;
                                        ++iter2;
                                        int word = boost::lexical_cast<int>(*iter1);
                                        int frequency = boost::lexical_cast<int>(*iter2);

                                        assert(word >= 1);
                                        // -1 for word id starts from 0
                                        new_doc.append_word(word - 1, frequency);
                                }
                                // randomly initilize the topic for each word
                                new_doc.random_init_topics(FLAGS_num_topics);
                                corpusLock.lock();
                                corpus.push_back(new_doc);
                                corpusLock.unlock();
                                doc_count++;
                        }
                        infile.close();
                        LOG(INFO) << "my id: " << FLAGS_my_id << " local doc size(): " << doc_count;
                }
                //###
                else
                {
                        HDFSManager hdfs_manager(my_node, nodes, config, zmq_context);
                        LOG(INFO) << "manager set up";
                        hdfs_manager.Start();
                        LOG(INFO) << "manager start";
                        LOG(INFO) << "Start Loading and Initializing the corpus";
                        hdfs_manager.Run([my_node, &corpus, &corpusLock](HDFSManager::InputFormat* input_format, int local_tid) {
                                        int count = 0;
                                        while (input_format->HasRecord()){
                                        auto record = input_format->GetNextRecord();
                                        LDADoc new_doc;
                                        // parse doc_id pair1 pair2 ...
                                        boost::char_separator<char> sep(" \n");
                                        boost::tokenizer<boost::char_separator<char>> tok(record, sep);
                                        boost::tokenizer<boost::char_separator<char>>::iterator iter = tok.begin();

                                        // For Debugging
                                        //LOG(INFO) << "record: " << (record);

                                        // the first is doc id
                                        int doc_id = boost::lexical_cast<int>(*iter);
                                        ++iter;

                                        // parse w1:f1 w2:f2 w3:f3 ...

                                        for (; iter != tok.end(); ++iter) {
                                        std::string word_fre_pair = boost::lexical_cast<std::string>(*iter);
                                        boost::char_separator<char> sep(":");
                                        boost::tokenizer<boost::char_separator<char>> tok2(word_fre_pair, sep);
                                        boost::tokenizer<boost::char_separator<char>>::iterator iter1, iter2 = tok2.begin();
                                        iter1 = iter2;
                                        ++iter2;
                                        int word = boost::lexical_cast<int>(*iter1);
                                        int frequency = boost::lexical_cast<int>(*iter2);

                                        assert(word >= 0);
                                        // -1 for word id starts from 0
                                        new_doc.append_word(word - 1, frequency);
                                        }

                                        // randomly initilize the topic for each word
                                        // set num_topics == 10 for testing
                                        //int num_topics = 10;
                                        new_doc.random_init_topics(FLAGS_num_topics);
                                        corpusLock.lock();
                                        corpus.push_back(new_doc);
                                        corpusLock.unlock();
                                        count++;
                                        }
                                        LOG(INFO) << "num of doc in this worker: " << count;
                        });
                        hdfs_manager.Stop();
                }
                LOG(INFO) << "Finish Loading and Initializing the corpus";
                auto load_data_end_time = std::chrono::steady_clock::now();

                std::chrono::duration<double> load_data_duration = load_data_end_time - load_data_start_time;
                LOG(INFO) << "my id: " << FLAGS_my_id << "Load Data Duration: " << load_data_duration.count();

                //  For Debugging
                //              for (int i = 0; i < corpus[3426].get_num_tokens(); i++)
                //                      LOG(INFO) << "["<<i<<":"<<corpus[3426].get_token(i)<<":"<<corpus[3426].get_topic(i)<<"]";

                //  2. Start Engine
                Engine engine(my_node, nodes);
                engine.StartEverything();

                //  3. Create Word-Topic Table

                const int LDATableId = 0;
                std::vector<third_party::Range> range;
                int num_topics = FLAGS_num_topics;
                int max_voc_id = FLAGS_max_voc_id;
                int lda_table_dim = ( max_voc_id + 1 ) * num_topics;
                int num_total_workers = nodes.size() * FLAGS_num_workers_per_node;
                int num_total_servers = nodes.size() * FLAGS_num_servers_per_node;
                LOG(INFO) << "num_total_servers:" << num_total_servers
                        << "\tnodes.size():" << nodes.size() << "\tFLAGS_num_servers_per_node:" << FLAGS_num_servers_per_node;

                for (int i = 0; i < num_total_servers - 1; ++ i) {
                        range.push_back({(lda_table_dim / num_total_servers * i), (lda_table_dim / num_total_servers * (i + 1))});
                }
                range.push_back({(lda_table_dim / num_total_servers * (num_total_servers - 1)), (uint64_t)lda_table_dim});
                LOG(INFO) << "lda_table_dim: " << lda_table_dim;
                ModelType model_type;
                if (FLAGS_kModelType == "ASP") {
                        model_type = ModelType::ASP;
                } else if (FLAGS_kModelType == "SSP") {
                        model_type = ModelType::SSP;
                } else if (FLAGS_kModelType == "BSP") {
                        model_type = ModelType::BSP;
                } else if (FLAGS_kModelType == "SparseSSP") {
                        model_type = ModelType::SparseSSP;
                } else {
                        LOG(FATAL) << "model type not specified";
                        CHECK(false) << "model type error: " << FLAGS_kModelType;
                }
                StorageType storage_type;
                if (FLAGS_kStorageType == "Map") {
                        storage_type = StorageType::Map;
                } else if (FLAGS_kStorageType == "Vector") {
                        storage_type = StorageType::Vector;
                } else {
                        LOG(FATAL) << "storage type not specified";
                        CHECK(false) << "storage type error: " << FLAGS_kStorageType;
                }
                SparseSSPRecorderType sparse_ssp_recorder_type;
                if (FLAGS_kSparseSSPRecorderType == "None") {
                        sparse_ssp_recorder_type = SparseSSPRecorderType::None;
                } else if (FLAGS_kSparseSSPRecorderType == "Map") {
                        sparse_ssp_recorder_type = SparseSSPRecorderType::Map;
                } else if (FLAGS_kSparseSSPRecorderType == "Vector") {
                        sparse_ssp_recorder_type = SparseSSPRecorderType::Vector;
                } else {
                        LOG(FATAL) << "sparse ssp recorder type not specified";
                        CHECK(false) << "sparse_ssp_storage type error: " << FLAGS_kSparseSSPRecorderType;
                }
                engine.CreateTable<float>(LDATableId, range,
                                model_type, storage_type, FLAGS_kStaleness, FLAGS_kSpeculation, sparse_ssp_recorder_type);

                // 3.1 Create LDA Stat Table
                const int LDAStatTableId = 1;
                std::vector<third_party::Range> stat_table_range;
                int lda_stat_table_dim = FLAGS_num_iters * 9;
                for (int i = 0; i < num_total_servers - 1; ++ i) {
                        stat_table_range.push_back({(uint64_t)(lda_stat_table_dim / num_total_servers * i), (uint64_t)(lda_stat_table_dim / num_total_servers * (i + 1))});
                }
                stat_table_range.push_back({(uint64_t)(lda_stat_table_dim / num_total_servers * (num_total_servers - 1)), (uint64_t)lda_stat_table_dim});
                //stat_table_range.push_back({0, 9}); //###
                //stat_table_range.push_back({9, 18}); //###
                engine.CreateTable<float>(LDAStatTableId, stat_table_range, ModelType::ASP, StorageType::Vector, FLAGS_kStaleness, FLAGS_kSpeculation, SparseSSPRecorderType::None);


                engine.Barrier();

                //  4. Construct ML Task
                LOG(INFO) << "Contruct ML Task";
                MLTask task;
                std::vector<WorkerAlloc> worker_alloc;
                for (auto& node : nodes) {
                        worker_alloc.push_back({node.id, (uint32_t)FLAGS_num_workers_per_node});  // each node has num_workers_per_node workers
                }
                task.SetWorkerAlloc(worker_alloc);
                task.SetTables({LDATableId, LDAStatTableId});  // Use word_topic_table: 0
                task.SetLambda(
                                [LDATableId, LDAStatTableId, &corpus, num_topics, max_voc_id, num_total_workers]
                                (const Info& info)
                                {
                                auto start_time = std::chrono::steady_clock::now();
                                LOG(INFO) << info.DebugString();
                                auto start_epoch_timer = std::chrono::steady_clock::now();
                                int worker_id = info.worker_id;
                                int local_id = info.local_id;
                                int local_num_doc = corpus.size();
                                int num_workers_per_node = FLAGS_num_workers_per_node;
                                int local_corpus_batch_size = local_num_doc / num_workers_per_node;
                                int first_doc_idx = local_id * local_corpus_batch_size;
                                int last_doc_idx = (local_num_doc - local_id * local_corpus_batch_size <= local_corpus_batch_size ) ? (local_num_doc - 1) : ((local_id + 1) * local_corpus_batch_size - 1);

                                if (first_doc_idx >= local_num_doc)
                                {
                                LOG(INFO) << info.DebugString() << ": no document can be assigned to this node. End.";
                                return;
                                }
                                //int last_doc_idx = (local_id + 1 == num_workers_per_node) ? (num_doc - 1) : ((local_worker_id + 1) * local_corpus_batch_size - 1);

                                std::vector<LDADoc>::const_iterator local_first = corpus.begin() + first_doc_idx;
                                std::vector<LDADoc>::const_iterator local_last = corpus.begin() + last_doc_idx;
                                std::vector<LDADoc> local_corpus_by_worker(local_first, local_last);
                                LOG(INFO) << info.DebugString() << " number of doc in this node: " << local_num_doc << " num_workers_per_node: " << num_workers_per_node
                                        << " local_corpus_batch_size: " << local_corpus_batch_size << " first_doc_idx: " << first_doc_idx <<  " last_doc_idx: " << last_doc_idx
                                        << " doc in this worker: " << local_corpus_by_worker.size();

                                if (FLAGS_kModelType == "SSP" || FLAGS_kModelType == "ASP" || FLAGS_kModelType == "BSP") {  // normal mode
                                        LOG(INFO) << info.DebugString() << ": Starting initialization of LDA table";
                                        init_tables(local_corpus_by_worker, info, LDATableId, num_topics, max_voc_id);

                                }

                                // -----------------------------------
                                // 1. statistic header only write once
                                if (worker_id == 0) {
                                        std::string resFile = FLAGS_result_write_path;
                                        std::ofstream ofs;
                                        ofs.open(resFile, std::ofstream::out | std::ofstream::app);
                                        ofs << FLAGS_input << " num_topics:"<< FLAGS_num_topics <<" num_workers_per_node:"<< FLAGS_num_workers_per_node << " num_total_workers: " << num_total_workers;
                                        ofs <<" kStaleness:" << FLAGS_kStaleness;
                                        ofs << "\nkSpeculation:" << FLAGS_kSpeculation << " kModelType:" << FLAGS_kModelType << " kSparseSSPRecorderType:" << FLAGS_kSparseSSPRecorderType << " kStorageType:" << FLAGS_kStorageType << "\n";
                                        ofs << " iter | pull_t | push_t | samplet | P+S+S | mailboxt | llh_time | llh\n";
                                        ofs.close();
                                }
                                // 2. -----------------Main logic ---------------
                                LOG(INFO) << info.DebugString() << ": Start Sampling";
                                float alpha = FLAGS_alpha;
                                float beta = FLAGS_beta;
                                float num_iterations = FLAGS_num_iters;
                                auto lda_table = info.CreateKVClientTable<float>(LDATableId);
                                auto lda_stat_table = info.CreateKVClientTable<float>(LDAStatTableId);
                                batch_training_by_chunk(local_corpus_by_worker, lda_table, lda_stat_table, num_total_workers, num_topics, max_voc_id, num_iterations, alpha, beta, info);

                                //------------------------------------------------
                                // 3.(For multiple epoch task) calculate the statistics
                                auto end_epoch_timer = std::chrono::steady_clock::now();
                                std::chrono::duration<double> one_epoch_time = end_epoch_timer - start_epoch_timer;
                                if (worker_id == 0 && FLAGS_num_epochs > 1) {
                                        std::string resFile = FLAGS_result_write_path;
                                        std::ofstream ofs;
                                        ofs.open(resFile, std::ofstream::out | std::ofstream::app);
                                        //ofs<<"epoch "<<info.get_current_epoch()<<" takes:"<<one_epoch_time.count()<<"s\n";
                                        LOG(INFO)<<"one_epoch_time:"<< one_epoch_time.count();
                                        ofs.close();
                                }

                                auto end_time = std::chrono::steady_clock::now();
                                std::chrono::duration<double> duration = end_time - start_time;
                                LOG(INFO) << info.DebugString() << "Task Completed! Total_time: " << duration.count() << " seconds";
                                });

                // 5. Run task
                engine.Run(task);

                engine.Barrier();
                //###
                MLTask task_output;
                std::vector<WorkerAlloc> worker_alloc_out;
                for (auto& node : nodes) {
                        worker_alloc_out.push_back({node.id, (uint32_t)FLAGS_num_workers_per_node});  // each node has num_workers_per_node workers
                }
                task_output.SetWorkerAlloc(worker_alloc_out);
                task_output.SetTables({LDATableId, LDAStatTableId});  // Use word_topic_table: 0
                task_output.SetLambda(
                                [LDATableId, LDAStatTableId]
                                (const Info& info){
                                if (info.worker_id == 0)
                                {
                                LOG(INFO) << info.DebugString() << "Start generating output";
                                auto lda_table = info.CreateKVClientTable<float>(LDATableId);
                                //prepare all_keys
                                std::vector<Key> all_keys;
                                std::vector<float> params;
                                int max_voc_id = FLAGS_max_voc_id;
                                int num_topics = FLAGS_num_topics;
                                for (int i = 0; i < (max_voc_id + 1) * FLAGS_num_topics; ++ i)
                                all_keys.push_back(i);
                                auto start_pull_time = std::chrono::steady_clock::now();
                                {
                                lda_table.Get(all_keys, &params);
                                }
                                CHECK_EQ(all_keys.size(), params.size());
                                auto end_pull_time = std::chrono::steady_clock::now();

                                std::ofstream of;
                                std::ios_base::iostate exceptionMask = of.exceptions() | std::ios::failbit;
                                of.exceptions(exceptionMask);
                                of.open("/data/opt/tmp/1155004171/output/output-word-topic.txt", std::ofstream::out | std::ofstream::app);

                                // print word topic table
                                for (int i = 0; i < max_voc_id; i++)
                                {
                                        std::string writeline = "";
                                        for (int j = 0; j < num_topics-1; j++)
                                        {
                                                writeline += boost::lexical_cast<std::string>((int)params[i * num_topics + j]);
                                                writeline += "\t";
                                        }
                                        writeline += boost::lexical_cast<std::string>((int)params[i * num_topics + num_topics - 1]);
                                        writeline += "\n";
                                        of << writeline;
                                }
                                // print topic summary table
                                std::string writeline = "";
                                for (int i = 0; i < num_topics-1; i++)
                                {
                                        writeline += boost::lexical_cast<std::string>((int)params[max_voc_id * num_topics + i]);
                                        writeline += "\t";
                                }
                                writeline += boost::lexical_cast<std::string>((int)params[max_voc_id * num_topics + num_topics - 1]);
                                writeline += "\n";
                                of << writeline;
                                of.close();
                                }

                                });
                engine.Run(task_output);
                // 6. Stop Engine
                engine.StopEverything();
        }

}  // namespace csci5570

int main(int argc, char** argv) {
        gflags::ParseCommandLineFlags(&argc, &argv, true);
        google::InitGoogleLogging(argv[0]);
        csci5570::Run();
}