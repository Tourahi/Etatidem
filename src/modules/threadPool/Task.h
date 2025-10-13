
#ifndef LAB_TASK_H
#define LAB_TASK_H

#include <functional>

class Task {
public:
    using callableT = std::function<void()>;

    enum class  typeT {
        STD, // Standard
        TERM, // Terminal
    };

    Task(callableT callable, typeT type)
        : type_{std::move(type)}
        , callable_{std::move(callable)}
    {}

    Task(const Task&) = delete;
    Task(Task&&) = delete;

    Task& operator=(const Task&) = delete;
    Task& operator=(Task&&) = delete;

    typeT type() const { return type_; }

    void operator()() const { callable_(); }

private:
    typeT type_;
    callableT callable_;
};

#endif //LAB_TASK_H