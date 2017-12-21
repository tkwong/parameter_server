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

    DEFINE_double(with_injected_straggler, 0.0, "injected straggler with its probability");
    DEFINE_double(with_injected_straggler_delay, 100, "injected straggler delay in ms (obsoleted)");
    DEFINE_double(with_injected_straggler_delay_percent, 100, "injected straggler delay in %");

<!-- TODO: more explaination -->

### Benchmark Utility
We implemented benchmark utility so that we can measure the actual time lapsed for some part of the code.
implemented in `lib/benchmark.hpp`
<!-- TODO: any significance? -->

### Reporting interface

We use GLOG with prefix [STAT_TYPE] to allow worker/server to report the time, and gathered into CSV format.
e.g. `VLOG(2) << "[STAT_TIME] " `
<!-- FIXME: seems this implementation is not so fancy.. -->


## 3. Application
<!-- TODO: Brief discussion on the algorithm is sufficient-->

### Logistic Regression
<!-- TODO:  -->
https://machinelearningmastery.com/implement-logistic-regression-stochastic-gradient-descent-scratch-python/

### Support Vector Machine
<!-- TODO:  -->
https://maviccprp.github.io/a-support-vector-machine-in-just-a-few-lines-of-python-code/

### Matrix Factorization
<!-- TODO:  -->
http://www.albertauyeung.com/post/python-matrix-factorization/

### Latent Dirichlet allocation
<!-- TODO:  Do we need that? -->

## 4. Conclusion
<!--  Suggested by TA about the additional features-->

### Final Design
<!-- TODO: Anything we need to address here instead of baseline -->

### Technical Challenges
<!-- TODO:  -->

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

