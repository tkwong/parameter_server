#include "server/util/progress_tracker.hpp"

#include "glog/logging.h"

namespace csci5570 {

void ProgressTracker::Init(const std::vector<uint32_t>& tids) {
    min_clock_ = 0;

    for (auto tid : tids)
        progresses_.insert(std::make_pair(tid, 0));
}

int ProgressTracker::AdvanceAndGetChangedMinClock(int tid) {
    bool isUniqueMin = IsUniqueMin(tid);
    progresses_.at(tid)++;
    return (isUniqueMin ? ++min_clock_ : -1);
}

int ProgressTracker::GetNumThreads() const {
    return progresses_.size();
}

int ProgressTracker::GetProgress(int tid) const {
    //LOG(INFO) << "Getting progress of " << tid << " which is " 
    //          << (CheckThreadValid(tid) ? "" : "not") << " valid";
    return progresses_.at(tid); // Throw an error if not found.
}

int ProgressTracker::GetMinClock() const {
    return min_clock_;
}

bool ProgressTracker::IsUniqueMin(int tid) const {
    if (progresses_.at(tid) != min_clock_) return false; //If this tid is not min
    
    int counter = 0;
    for (auto it=progresses_.begin(); it!=progresses_.end(); it++)
    {
        if (it->second == min_clock_) counter++;
        if (counter > 1) break;
    }
    
    return counter == 1;
}

bool ProgressTracker::CheckThreadValid(int tid) const {
    return progresses_.find(tid) != progresses_.end();
}

}  // namespace csci5570
