#include "threadpool.hpp"

#include <chrono>

#include <future>
#include "myFuture.hpp"

void func(int a, int b, std::mutex *mtxCout)
{
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(30ms);
    {
        std::lock_guard<std::mutex> lg(*mtxCout);
        std::cout << "\t\t[inside task .. thread " << std::this_thread::get_id() << "]" << std::endl;
    }
}

template <typename T>
void func_my_promise(ReturnObjectDelivery<T> promise, T a, T b, std::mutex *mtxCout)
{
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(30ms);
    {
        std::lock_guard<std::mutex> lg(*mtxCout);
        std::cout << "\t\t[inside task .. thread " << std::this_thread::get_id() << "]" << std::endl;
    }
    T ret = a + b;
    promise.set_value(ret);
}

template <typename T>
void func_std_promise(std::promise<T> &&promise, T a, T b, std::mutex *mtxCout)
{
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(30ms);
    {
        std::lock_guard<std::mutex> lg(*mtxCout);
        std::cout << "\t\t[inside task .. thread " << std::this_thread::get_id() << "]" << std::endl;
    }
    T ret = a + b;
    promise.set_value(ret);
}

template <typename T>
T func_std_package(T a, T b, std::mutex *mtxCout)
{
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(30ms);
    {
        std::lock_guard<std::mutex> lg(*mtxCout);
        std::cout << "\t\t[inside task .. thread " << std::this_thread::get_id() << "]" << std::endl;
    }
    T ret = a + b;
    return ret;
}

int main(int argc, char **argv)
{
    constexpr int threadCount = 4;
    constexpr int taskCount = 4;

    ThreadPool tp(threadCount);

    // lambda version
    if (false)
    {
        for (int i = 0; i < taskCount; ++i)
        {
            tp.pushTask(std::function<void()>{
                [mtx = &(tp.mtxCout)]()
                {
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(30ms);
                    {
                        std::lock_guard<std::mutex> lg(*mtx);
                        std::cout << "\t\t[inside task .. thread " << std::this_thread::get_id() << "]" << std::endl;
                    }
                }});
        }
    }

    // bind version
    if (false)
    {
        for (int i = 0; i < taskCount; ++i)
        {
            int a, b;
            std::function<void()> task = std::bind(func, a, b, &(tp.mtxCout));
            tp.pushTask(task);
        }
    }

    // custom classes { ReturnObject, ReturnObjectDelivery } version
    if (false)
    {
        std::vector<ReturnObject<int>> futures(taskCount);

        for (int i = 0; i < taskCount; ++i)
        {
            int a = 10, b = 30;
            ReturnObjectDelivery<int> promise; // copyable
            futures[i].connectToROD(&promise);

            // std::function<void()> task = std::bind(func_future<int>, promise, a, b, &(tp.mtxCout));
            std::function<void()> task = [promise, a, b, &tp]()
            { func_my_promise<int>(promise, a, b, &(tp.mtxCout)); };
            tp.pushTask(task);
        }

        // print return value
        for (int idx = 0; idx < futures.size(); ++idx)
        {
            int ret = futures[idx].get();
            {
                std::lock_guard<std::mutex> lg(tp.mtxCout);
                std::cout << "task " << idx + 1 << " return value = " << ret << std::endl;
            }
        }
    }

    // shared_ptr, packaged_task, std::future version
    if (true)
    {
        std::vector<std::future<int>> futures;

        for (int i = 0; i < taskCount; ++i)
        {
            int a = 10, b = 30;
            /*
                std::packaged_task
                    : to save states with std::function<void()>
                    : std::function<void()> is stateless object

                std::shared_ptr
                    : to copy capture std::packaged_task
                    : std::packaged_task is move-only object
                    : + also object's lifetime control
            */
            auto sp_packaged_task = std::make_shared<std::packaged_task<int()>>(
                std::bind(func_std_package<int>, a, b, &(tp.mtxCout)));
            std::future<int> future = sp_packaged_task->get_future();
            futures.emplace_back(std::move(future));

            std::function<void()> task = [sp_packaged_task]()
            { (*sp_packaged_task)(); };
            tp.pushTask(task);
        }

        // print return value
        for (int idx = 0; idx < futures.size(); ++idx)
        {
            auto &&a = futures[idx].get();
            {
                std::lock_guard<std::mutex> lg(tp.mtxCout);
                std::cout << "task " << idx + 1 << " return value = " << a << std::endl;
            }
        }
    }
}