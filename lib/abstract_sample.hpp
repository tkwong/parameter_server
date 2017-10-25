#pragma once

namespace csci5570 {
namespace lib {

template <typename Feature, typename Label>
class AbstractSample {
    public:
        virtual void addFeature(int index, Feature feature) = 0;
        virtual void addLabel(Label label) = 0;
}; // class AbstractSample

}  // namespace lib
}  // namespace csci5570
