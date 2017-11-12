#include <thread>
#include <iostream>
#include <math.h>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "lib/labeled_sample.hpp"
#include "lib/abstract_data_loader.hpp"

using namespace csci5570;

typedef lib::LabeledSample<double, int> Sample;
typedef std::vector<Sample> DataStore;

double predict(Sample sample, std::vector<double> model)
{
    double yhat = model.at(0);
    for (Eigen::SparseVector<double>::InnerIterator it(sample.features); it; ++it)
        yhat += model.at(it.index()) * it.value();
    return 1.0 / (1.0 + exp(-yhat));
}

std::vector<double> train(DataStore data, double learning_rate)
{
    std::vector<double> model(124); // FIXME: temp hard-code
    int i=0;
    for(Sample sample : data)
    {
        double yhat = predict(sample, model);
        double error = (sample.label > 0 ? 1 : 0) - yhat;
        // if (i%1000==0) std::cout << "Error Square: " << error*error << std::endl;
	
        double gradient = model.at(0);
        for (Eigen::SparseVector<double>::InnerIterator it(sample.features); it; ++it)
            gradient += it.value() * error;

        model.at(0) += learning_rate * gradient;

        for (Eigen::SparseVector<double>::InnerIterator it(sample.features); it; ++it)
            model.at(it.index()) += learning_rate * gradient;
        i++;
    }
    return model;
}

double test(DataStore data, std::vector<double> model)
{
    int correct = 0;
    for (Sample sample : data)
    {
        double estimate = predict(sample, model);
        int est_class = (estimate > 0.5 ? 1 : 0);
        int answer = (sample.label > 0 ? 1 : 0);
        if (est_class == answer) correct++;
    }
    std::cout << "Accuracy: " << correct << " out of " << data.size() 
              << " " << float(correct)/data.size()*100 << " percent" << std::endl;
}

int main(int argc, char** argv)
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_stderrthreshold = 0;
    FLAGS_colorlogtostderr = true;

    DataStore train_data;
    lib::AbstractDataLoader<Sample, DataStore>::load<std::function<Sample(boost::string_ref, int)>>(
        "hdfs://proj10:9000/datasets/classification/a9/", 123, 
        lib::Parser<Sample, DataStore>::parse_libsvm, &train_data);

    DataStore test_data(train_data.begin(), train_data.begin()+2561);
    train_data.erase(train_data.begin(), train_data.begin()+2561);

    std::cout << "Train data size: " << train_data.size() << std::endl;
    std::cout << "Test data size: " << test_data.size() << std::endl;

    auto model = train(train_data, 0.0001);
    std::cout << "Model[0]: " << model.at(0) << std::endl;

    test(test_data, model);

    return 0;
}
