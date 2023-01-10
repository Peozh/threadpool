#include "threadpool.hpp"

#include <chrono>

// #include <future>
#include "myFuture.hpp"

void func(int a, int b, std::mutex* mtxCout)
{
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(30ms);
    {
        std::lock_guard<std::mutex> lg(*mtxCout);
        std::cout << "\t\t[inside task .. thread " << std::this_thread::get_id() << "]" << std::endl;
    }
}

template <typename T>
void func_future(ReturnObjectDelivery<T> promise, T a, T b, std::mutex* mtxCout)
{
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(30ms);
    {
        std::lock_guard<std::mutex> lg(*mtxCout);
        std::cout << "\t\t[inside task .. thread " << std::this_thread::get_id() << "]" << std::endl;
    }
    T ret = a+b;
    promise.set_value(ret);
}

int main(int argc, char** argv)
{
    constexpr int threadCount = 4;
    constexpr int taskCount = 4;

    ThreadPool tp(threadCount);

    // lambda version
    if (false) { 
        for (int i = 0; i < taskCount; ++i) 
        {
            tp.pushTask(std::function<void()> { [mtx = &(tp.mtxCout)]() { 
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
    if (false) {  
        for (int i = 0; i < taskCount; ++i) 
        {
            int a, b;
            std::function<void()> task = std::bind(func, a, b, &(tp.mtxCout));
            tp.pushTask(task);
        }
    }

    // myFuture version
    if (true) {
        std::vector<ReturnObject<int>> returnObjects(taskCount);

        for (int i = 0; i < taskCount; ++i) 
        {
            int a = 10, b = 30;
            ReturnObjectDelivery<int> promise;
            returnObjects[i].connectROD(&promise);

            // std::function<void()> task = std::bind(func_future<int>, promise, a, b, &(tp.mtxCout));
            std::function<void()> task = [promise, a, b, &tp]() { func_future<int>(promise, a, b, &(tp.mtxCout)); };
            tp.pushTask(task);
        }

        // print myFuture version return value
        for (int idx = 0; idx < returnObjects.size(); ++idx) 
        {
            int a = returnObjects[idx].get();
            {
                std::lock_guard<std::mutex> lg(tp.mtxCout);
                std::cout << "task " << idx + 1 << " return value = " << a << std::endl;
            }
        }
    }
}