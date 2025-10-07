
#ifndef WORK_H
#define WORK_H
#include <functional>

namespace Thread {

  class work {
  public:
    using callable = std::function<void()>;

    // Types of work
    enum class typeT {
      STANDARD,
      TERMINAL
    };

    work(typeT type, callable c)
      : type_{std::move(type)},
        callable_{std::move(c)}
    {}

    work(const work&) = delete;
    work(work&&) = default;

    work& operator=(const work&) = delete;
    work& operator=(work&&) = default;

    typeT type() const { return type_; }

    void operator()() const { callable_(); }

  private:
    typeT type_;
    callable callable_;
  };

}

#endif //WORK_H
