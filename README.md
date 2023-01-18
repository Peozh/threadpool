# threadpool
my threadpool with condition_variable, mutex, unique_lock, lock_guard, atomic

## 1. Naive Version
```c++
#include "threadpool.hpp"

void func(...)
{
    ...
}

constexpr int threadCount = 4;
ThreadPool tp(threadCount);
```
#### push task (with bind)
```c++
std::function<void()> task = std::bind(func, ...);
tp.pushTask(task);
```
#### push task (with lambda)
```c++
tp.pushTask(std::function<void()> { [](){ ... } });
```

## 2. Custom Version - future & promise
```c++
#include "threadpool.hpp"
#include "myFuture.hpp"

template <typename T>
void func_my_promise(ReturnObjectDelivery<T> promise, ...)
{
    ...
    T ret = a + b;
    promise.set_value(ret);
}

constexpr int threadCount = 4;
constexpr int taskCount = 4;
ThreadPool tp(threadCount);

std::vector<ReturnObject<int>> futures(taskCount);
```
#### push task (with bind)
```c++
ReturnObjectDelivery<int> promise;
futures[i].connectToROD(&promise);

std::function<void()> task = std::bind(func_my_promise<int>, promise, ...);
tp.pushTask(task);
```
#### push task (with lambda)
```c++
ReturnObjectDelivery<int> promise;
futures[i].connectToROD(&promise);

std::function<void()> task = [promise, ...]() { func_my_promise<int>(promise, ...); };
tp.pushTask(task);
```
#### get return value
```c++
auto ret = futures[idx].get();
```
## 3. Standard Version - std::future & std::packaged_task
```c++
#include "threadpool.hpp"
#include <future>

template <typename T>
T func_std_package(T a, T b, std::mutex *mtxCout)
{
    ...
    T ret = a + b;
    return ret;
}

constexpr int threadCount = 4;
constexpr int taskCount = 4;
ThreadPool tp(threadCount);

std::vector<std::future<int>> futures;
```
#### push task
```c++
auto sp_packaged_task = std::make_shared<std::packaged_task<int()>>(
    std::bind(func_std_package<int>, a, b, &(tp.mtxCout)));

futures.emplace_back(sp_packaged_task->get_future());

std::function<void()> task = [sp_packaged_task](){ (*sp_packaged_task)(); };
tp.pushTask(task);
```
#### get return value
```c++
auto ret = futures[idx].get();
```
## 4. Example Output [threadCount = 4, taskCount = 4]
```shell
thread idx = 0 generated & detached
        thread 2 starting...
        thread 2 waiting...
thread idx = 1 generated & detached
        thread 3 starting...
        thread 3 waiting...
thread idx = 2 generated & detached
        thread 4 starting...
        thread 4 waiting...
thread idx = 3 generated & detached
        thread 5 starting...
        thread 5 waiting...
        thread 5 awaken !
                        task pushed
                        task pushed
                        task pushed
        thread 2 awaken !
        thread 2 tasking...
        thread 5 tasking...
        thread 4 awaken !
        thread 4 tasking...
        thread 3 awaken !
                        task pushed
        thread 3 tasking...
                [inside task .. thread 3]
                [inside task .. thread 4]
        thread 4 task (done)
        thread 4 waiting...
        thread 3 task (done)
                [inside task .. thread 5]
        thread 5 task (done)
        thread 3 waiting...
                [inside task .. thread 2]
task 1 return value = 40
task 2 return value = 40
        thread 2 task (done)
        thread 5 waiting...
task 3 return value = 40
        thread 2 waiting...
task 4 return value = 40
releaseWorkers()...
all task assigned
        thread 2 awaken !
        thread 2 terminated
        thread 3 awaken !
        thread 3 terminated
all task done
        thread 4 awaken !
        thread 5 awaken !
        thread 4 terminated
        thread 5 terminated
all thread terminated
```
