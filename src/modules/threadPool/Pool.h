#ifndef LAB_POOL_H
#define LAB_POOL_H

#include "Task.h"

#include <boost/lockfree/queue.hpp>

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>


class Pool {

public:
    enum class methodType {
        SYNC,
        ASYNC
    };
    template <typename T>
    using callableType = std::function<T()>;
    using sizeType = std::size_t;

private:
    using taskType = Task;
    using queueType = boost::lockfree::queue<taskType*>;
    class noFutureTask { friend class Pool; noFutureTask() {} };

public:
    Pool(sizeType workerCount = std::thread::hardware_concurrency() / 2);
    ~Pool();

    static const noFutureTask noFutureTag;

    template <typename T>
    std::future<T> postTask(callableType<T> callable) {
        return postTask(taskType::typeT::STD, std::move(callable));
    }

    template <typename T>
    void postTask(callableType<T> callable, noFutureTask) {
        return postTask(taskType::typeT::STD, std::move(callable), noFutureTag);
    }

    void joinAll();

    void setWorkerCount(sizeType n, methodType method = methodType::ASYNC);

    sizeType getWorkerCount() const { return workerCount_; };

    sizeType getTaskQueueSize() const { return taskQueueSize_.load(std::memory_order_relaxed); }

    void increaseWorkerCount(sizeType n);

    void decreaseWorkerCount(sizeType n = std::numeric_limits<sizeType>::max(),
        methodType method = methodType::ASYNC);

private:
    using mutexT = std::mutex;
    using cvT = std::condition_variable;
    sizeType workerCount_;
    mutable mutexT taskSignalMutex_;
    cvT taskSignal_;
    queueType taskQueue_;
    std::atomic<sizeType> taskQueueSize_;

    Pool(Pool&&) = delete;
    Pool(Pool const&) = delete;
    Pool& operator=(Pool&&) = delete;
    Pool& operator=(Pool const&) = delete;

    template <typename T>
    std::future<T> postTask(taskType::typeT type, callableType<T> callable);
    template <typename T>
    void postTask(taskType::typeT type, callableType<T> callable, noFutureTask);

    template <typename T>
    static void callHelper(callableType<T> callable, std::shared_ptr<std::promise<T>> promise);
    template <typename T>
    static void callHelper(callableType<T> callable);

    void push(std::unique_ptr<taskType> task);

    void workerMain();

};

template<typename T>
void Pool::callHelper(callableType<T> callable, std::shared_ptr<std::promise<T>> promise) {
    auto&& val = callable();
    promise->set_value(std::move(val));
}

template<>
inline void Pool::callHelper(callableType<void> callable, std::shared_ptr<std::promise<void>> promise) {
    callable();
    promise->set_value();
}

template<typename T>
void Pool::callHelper(callableType<T> callable) {
    callable();
}

inline void Pool::push(std::unique_ptr<taskType> task) {
    taskQueue_.push(task.release());

    {
        std::lock_guard<mutexT> taskSignalLock(taskSignalMutex_);
        taskQueueSize_.fetch_add(1, std::memory_order_relaxed);
    }
    taskSignal_.notify_one();
}

template<typename T>
std::future<T> Pool::postTask(taskType::typeT type, callableType<T> callable) {
    auto promise = std::make_shared<std::promise<T>>();
    auto future = promise->get_future();

    std::function<void()> func = std::bind(
        static_cast<void(*)(callableType<T>, std::shared_ptr<std::promise<T>>)>(&Pool::callHelper<T>),
        std::move(callable),
        std::move(promise));

    std::unique_ptr<taskType> task{new taskType{std::move(func), std::move(type)}};

    push(std::move(task));

    return future;
}

template<typename T>
void Pool::postTask(taskType::typeT type, callableType<T> callable, noFutureTask) {
    std::function<void()> func = std::bind(
        static_cast<void(*)(callableType<T>)>(&Pool::callHelper<T>),
        std::move(callable));

    std::unique_ptr<taskType> task{new taskType{std::move(func), std::move(type)}};

    push(std::move(task));
}

inline void Pool::workerMain() {
    while (true) {
        taskType* taskPtr;

        while (taskQueue_.pop(taskPtr)) {
            const std::unique_ptr<taskType> task(taskPtr);

            taskQueueSize_.fetch_sub(1, std::memory_order_relaxed);

            (*task)();

            if (task->type() == taskType::typeT::TERM)
                return;
        }

        std::unique_lock<mutexT> taskSignalLock(taskSignalMutex_);
        taskSignal_.wait(taskSignalLock, [this]() { return taskQueueSize_.load(std::memory_order_relaxed) > 0; });
    }
}



#endif //LAB_POOL_H