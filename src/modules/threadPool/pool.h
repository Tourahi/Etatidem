#ifndef POOL_H
#define POOL_H

#include "work.h"

#include <boost/lockfree/queue.hpp>

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>

namespace Thread {

  class pool {
  public:

    enum class methodT {
      SYNC,
      ASYNC
    };

    template <class T>
    using callableT = std::function<T()>;
    using sizeType = std::size_t;

  private:
    using workT = work;
    // Check out:
    // https://preshing.com/20120612/an-introduction-to-lock-free-programming/
    using queueT = boost::lockfree::queue<workT>;
    class noFutureT { friend class pool; noFutureT() {} };

  public:
    pool(sizeType workerCount = std::thread::hardware_concurrency() / 2);
    ~pool();

    /**
   * \brief Posts a work to the pool for getting processed.
   *
   * if there are no threads left (i.e. you called pool::join_all(); prior to
   * this function) all the works you post gets enqueued. if you spawn new threads in
   * the future, they will be executed then.
   *
   * ThreadSafe: <b>true</b>
   */
    template <class T>
    std::future<T> postWork(callableT<T> callable) {
      return postWork(workT::typeT::STANDARD, std::move(callable));
    }

    template <class T>
    void postWork(callableT<T> callable) {
      return postWork(workT::typeT::STANDARD, std::move(callable), noFutureTag);
    }

  private:
    using mutexT = std::mutex;
    using cvT = std::condition_variable;

    pool(pool&&) = delete;
    pool(pool const&) = delete;
    pool& operator=(pool&&) = delete;
    pool& operator=(pool const&) = delete;

    template <class T>
    std::future<T> postWork(workT::typeT type, callableT<T> callable);

    template <class T>
    void postWork(workT::typeT type, callableT<T> callable, noFutureT);

    template <class T>
    static void callHelper(callableT<T> callable, std::shared_ptr<std::promise<T>> promise);

    template <class T>
    static void callHelper(callableT<T> callable);

    void push(std::unique_ptr<workT> work);

    void workerMain();

  public:
    static const noFutureT noFutureTag;

  private:
    sizeType workerCount_;
    mutable mutexT workSignalMutex_;
    cvT workSignal_;
    queueT workQueue_;
    std::atomic<sizeType> workQueueSize_;
  };

  template <class T>
  void pool::callHelper(callableT<T> callable, std::shared_ptr<std::promise<T>> promise) {
    auto&& val = callable();
    promise->set_value(val);
  }

  template <>
  inline void pool::callHelper<void>(callableT<void> callable, std::shared_ptr<std::promise<void>> promise) {
    callable();
    promise->set_value();
  }

  template <class T>
  void pool::callHelper(callableT<T> callable) {
    callable();
  }

}


#endif //POOL_H
