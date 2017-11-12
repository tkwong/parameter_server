#pragma once

#include "lib/abstract_sample.hpp"
#include "lib/Eigen/Sparse"

namespace csci5570 {
namespace lib {

// Consider both sparse and dense feature abstraction
// You may use Eigen::Vector and Eigen::SparseVector template
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
        // std::ostream& operator<<(std::ostream& os, const LabeledSample& obj)
        // {
        //     // write obj to stream
        //     return os;
        // }

};  // class LabeledSample

}  // namespace lib
}  // namespace csci5570
