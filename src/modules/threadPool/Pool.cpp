#include "Pool.h"

#include <algorithm>
#include <functional>
#include <future>
#include <vector>


const Pool::noFutureTask Pool::noFutureTag;

Pool::Pool(sizeType workerCount)
    : workerCount_{0}
    , taskQueue_{0}
    , taskQueueSize_{0}
{
    increaseWorkerCount(workerCount);
}

Pool::~Pool() {
    joinAll();
}

void Pool::joinAll() {
    decreaseWorkerCount(std::numeric_limits<sizeType>::max(), methodType::SYNC);
}

void Pool::increaseWorkerCount(sizeType n) {
    workerCount_ += n;

    while (n-- > 0) {
        std::thread thread{std::bind(&Pool::workerMain, this)};
        thread.detach();
    }
}

void Pool::setWorkerCount(sizeType n, methodType method) {
    if (getWorkerCount() < n) {
        increaseWorkerCount(n - getWorkerCount());
    } else {
        decreaseWorkerCount(getWorkerCount() - n, method);
    }
}

void Pool::decreaseWorkerCount(sizeType n, methodType method) {
    std::vector<std::future<void>> futures;
    n = std::min(n, getWorkerCount());

    workerCount_ -= n;

    if (method == methodType::SYNC)
        futures.reserve(n);

    while (n > 0) {
        --n;

        if (method == methodType::SYNC) {
            futures.emplace_back(postTask<void>(taskType::typeT::TERM, []() {}));
        } else {
            postTask<void>(taskType::typeT::TERM, []() {}, noFutureTag);
        }
    }

    for (auto& f : futures)
        f.get();

}