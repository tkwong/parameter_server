#pragma once

#include "lib/abstract_sample.hpp"
#include "eigen3/Eigen/Sparse"

namespace csci5570 {
namespace lib {

// Consider both sparse and dense feature abstraction
// You may use Eigen::Vector and Eigen::SparseVector template
template <typename Feature, typename Label>
class LabeledSample : public AbstractSample<Feature,Label> {
    public:
        Eigen::SparseVector<Feature> features;
        Label label;
        
        void addFeature(int index, Feature feature) override
        {
            if(features.size() <= index)
                features.conservativeResize(index*2);
            features.coeffRef(index) = feature;
        }
        void addLabel(Label label)
        {
            this->label = label;
        }
        
};  // class LabeledSample

}  // namespace lib
}  // namespace csci5570
