# CSCI5570 Course Project Report

## 1. Baseline Features

### Functionality of workers (20%)

#### Model Accessing (10%)
[KVClientTable](worker/kv_client_table.hpp) provides a user interface to access the model parameters on the worker side, and issues push/pull requests.

Implemented `MapStorage` and `VectorStorage` and both support sparse and dense models.

#### Training Data Abstraction (5%)

`AbstractDataLoader` and `AbstractAsyncDataLoader` provide a user interface to access the training/testing data (loaded from distributed file systems HDFS)
Implemented `lib/labeled_sample.hpp` with `Eigen::Vector` and `Eigen::SparseVector` for the training data abstraction.

`lib/labeled_sample.hpp`

    template <typename Feature, typename Label>
    class LabeledSample : public AbstractSample<Feature,Label> {
        public:
            Eigen::SparseVector<Feature> features;
            Label label;

            LabeledSample(int n_features = 0) {
              if (n_features > 0)
                features.conservativeResize(n_features+1);
            }

            void addFeature(int index, Feature feature) override
            {
                features.coeffRef(index) = feature;
            }
            void addLabel(Label label) override
            {
                this->label = label;
            }

    };  // class LabeledSample

<!-- FIXME: We implemented in Abstract Interface, definitely it is a NONO. -->

Implemented DataLoader and AsyncDataLoader for loading the data.

for the Async Data Loader, we have used multi-producer, multi-consumer lock-free queue implemented my ModdyCamel.
<http://moodycamel.com/blog/2014/a-fast-general-purpose-lock-free-queue-for-c++>. Imported in `lib/concurrentqueue.h` and `blockingconcurrentqueue.h`

#### Application Logic Execution (5%)

`MLTask` provide the interface to set the user defined function and `Engine->Run()` provides a user interface to execute user defined function

### Functionality of servers (30%)

#### Request handling (5%)
The `Mailbox` is implemented as a bottom layer communication module, see `comm/mainbox.cpp`, `comm/sender.cpp`.
`ServerThreads`, `WorkerThreads` implemented to handles push/pull requests from server/workers, see `server/server_thread.cpp` and `worker/worker_thread.cpp`

#### Model manipulation (10%)

`AbstractPartitionManager` maintains a partition of model parameters.

`ConsistentHashingPartitionManager` implemented using Jump Consistent Hash.
Jump consistent hash is a fast, minimal memory, consistent hash algorithm developed by Google <https://arxiv.org/ftp/arxiv/papers/1406/1406.2294.pdf>.
Implemented in `base/consistent_hashing_parition_manager.hpp`

    // Jump Consistent Hash : https://arxiv.org/ftp/arxiv/papers/1406/1406.2294.pdf
    static int32_t JumpConsistentHash(uint64_t key, int32_t num_buckets) {
        int64_t b = -1, j = 0;
        while (j < num_buckets) {
            b = j;
            key = key * 2862933555777941757ULL + 1;
            j = (b + 1) * (double(1LL << 31) / double((key >> 33) + 1));
        }
        return b;
    }

`RangePartitionManager` also implemented using range based partitioning, implemented in `base/range_partition_manager.hpp`

provides access and applies updates to the parameters.

#### Consistency control (15%): 5% for each consistency model

supported three consistency models (`BSPModel`, `SSPModel` and `ASPModel`), Implemented in `server/consistency/bsp_model.hpp`, `server/consistency/asp_model.hpp` and `server/consistency/ssp_model.hpp` respectively.

<!-- FYI: Completeness and robustness (10%): the program has no obvious bug and can run the correct logic -->

## 2. Addition Features - Addressing straggler problems

We have chosen to add a feature to mitigate straggler problem.

### Workload Scheduler using iteration time feedback from worker

The aim of Scheduler is to solve the Straggler among workers. The worker thread will record the time used by each worker for an iteration and a scheduler (one of the worker thread will be assigned) will use this to calculate the workload of each worker in the following iteration. The worker thread will get the workload suggested by scheduler and execute the workload.

2 addition tables in engine is created for this purpose. `TimeTable` and `WorkloadTable`.

![Scheduler Structure](doc/scheduler_structure.png "Scheduler Structure")

The Engine will create and Initialize `TimeTable` and `WorkloadTable` when `engine->Run()` is called.

The Scheduler will be implemented by application UDF using `task.SetScheduler()`, similar to `task.SetLambda()`.

### Injected Straggler Model

We support random straggler delay in application, for example, in `logistic_regression.cpp`, we add

    DEFINE_double(with_injected_straggler_delay_percent, 100, "injected straggler delay in %");
    DEFINE_bool(activate_transient_straggler, true, "activate transient straggler");
    DEFINE_bool(activate_permanent_straggler, true, "activate permanemt straggler");

`with_injected_straggler_delay_percent` is the setting to inject straggler delay to the actual execution time of the iteration. for example, the actual iteration time is 1s, then the 100% delay will incur 1s sleep before Add/Clock

`activate_transient_straggler` is to activate the injection of transisent straggler. The transisent straggler is now designed to inject delay to worker ID == 1 and iteration count from 50 to 75. 

    if (FLAGS_activate_transient_straggler && info.worker_id == 1 && i >= 50 && i < 75)

`activate_permanent_straggler` is to activate the injection of permanet straggler. The permanent straggler is now designed to inject delay to worker ID == 3 and to all iteration. 

    if (FLAGS_activate_permanent_straggler && info.worker_id == 3)

### Benchmark Utility
We implemented benchmark utility so that we can measure the actual time lapsed for some part of the code.
implemented in `lib/benchmark.hpp`

It is designed to collecting datapoint for benchmark and instrument the execution time for statistic or straggler delay calculation. 

It provide the call to `start_measure`, `stop_measure`, `pause_measure` and `resume_measure` for collecting one data point. Whenever calling a `start_measure`, it will start a new data point and util `stop_measure`, it will record a data point. With all the datapoint is stored in Benchmark object, it provide a `mean`, `std_dev`, `sum` or `total` for all data point, also you can retrieve last n datapoint recorded through `last(n)` function. 

It also provide the utility call to inject a lambda function and calculate the time lapsed. more please see the `lib/benchmark_test.cpp` for its functionalities and testing. 

### Reporting interface

We use GLOG with prefix [STAT_TYPE] to allow worker/server to report the time, and gathered into CSV format.
e.g. `VLOG(2) << "[STAT_TIME] " `

Currently we use a runner script to extract the running statistic. `scripts/logistic_regression.py`, `scripts/svm.py`, `scripts/mf.py` will launch the application in cluster through SSH, and it will collect all the log into a shared folder (default to logs/[timestamp]). When the app is completed, the script will select the statistic log and distribute to CSV with corresponding file. e.g. stat_time, stat_work, stat_iter.. 

    print "Preparing statistic file"
    for type in ['wait' , 'proc', 'iter', 'work', 'time', 'totl'] :
        os.system("grep STAT_" + type.upper() + " " + log_dir + "/output.* | awk '{print $5 \",\" $6}' > " + log_dir + "/stat_" + type + ".csv")

After generating the stat file, it has a function to email the stat file to the user. :)

    email=os.getlogin() + "@link.cuhk.edu.hk"
    print "Email result? [yes or no]"
    yes = {'yes','y', 'ye', ''}
    no = {'no','n'}
    #choice = raw_input().lower()
    choice = "yes"
    while True :
        if choice in yes :
            print "Sending Email to [" + email + "]"
            os.system("mail -s 'Result for" + log_dir + "' -a " + log_dir + "/stat_iter.csv   -a " + log_dir + "/stat_proc.csv   -a " + log_dir +      "/stat_time.csv   -a " + log_dir + "/stat_wait.csv   -a " + log_dir + "/stat_work.csv  "+email+" < /dev/null")
            break
        elif choice in no:
            break
        else:
          print "Please respond with 'yes' or 'no'"
    print "Finished"

## 3. Application

Basically we are porting the other implementation (majorly python) and implement into our parameter server impelementation. 

### Logistic Regression

The implementation is based on the SGD approach, basically porting the python implementation to our implementation <https://machinelearningmastery.com/implement-logistic-regression-stochastic-gradient-descent-scratch-python/> 

### Support Vector Machine

The SVM implementation is also based on the python implementation here: 
<https://maviccprp.github.io/a-support-vector-machine-in-just-a-few-lines-of-python-code/>

### Matrix Factorization

The Matrix Factorization implementation is also based on the python implementation here: 
<http://www.albertauyeung.com/post/python-matrix-factorization/>

## 4. Conclusion

### Final Design
<!-- TODO: Anything we need to address here instead of baseline -->


### Technical Challenges
We have designed a scheduling algorithm for the scheduler.

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

The algorithm works as follow,
1. Find the smallest processing time as min_time.
2. Find the workers that have processing time larger than min_time * update_threshold (we set this to 1.5).
3. Take away some workloads from these workers, the number of workloads taking away is calculated by current workload * rebalance_workload (we set this to 0.2). Since number of workloads must be integer, we round the result.
4. Distribute these workloads to the remaining workers in ascending order of their processing time, try to distribute evenly while make sure that the total workloads is unchanged.

This algorithm can handle both permanent and transient stragglers. This is because when a permanent or transient straggler is found, its workloads will be reduced by the algorithm. It also works when the transient straggler is recovered, because the recovered worker will have the smallest processing time and caused others to be treated as stragglers. Therefore, the algorithm will give back some workloads to this worker.


### Evaluation on the performance
<!-- TODO:  Will post the plot here -->

#### Workload update time Analysis
![](doc/job_time_vs_workload_update_rate.png)

#### Logistic Regression, With injected Permanent Straggler
![](doc/perm_straggler_no_scheduler.png)

#### Logistic Regression, With injected Permanent Straggler and Scheduler
![](doc/perm_straggler_with_scheduler.png)

#### Logistic Regression, With injected Transient Straggler
![](doc/tran_straggler_no_scheduler.png)

#### Logistic Regression, With injected Transient Straggler and Scheduler
![](doc/tran_straggler_with_scheduler.png)

#### Suport Vector Machine, With Scheduler and Without Scheduler

#### Matrix Factorization, With Scheduler and Without Scheduler

