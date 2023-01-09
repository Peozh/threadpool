#include "threadpool.hpp"

#include <chrono>

void bindableFunction(int a, int b, std::mutex* mtx)
{
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(30ms);
    {
        std::lock_guard<std::mutex> lg(*mtx);
        std::cout << "\t\t[inside task .. thread " << std::this_thread::get_id() << "]" << std::endl;
    }
}

int main(int argc, char** argv)
{
    ThreadPool tp(4);

    for (int i = 0; i < 10; ++i) 
    {
        if (false) { // lambda version
            tp.pushTask(std::function<void()> { [mtx = &(tp.mtxCout)]() { 
                using namespace std::chrono_literals; 
                std::this_thread::sleep_for(30ms);
                {
                    std::lock_guard<std::mutex> lg(*mtx);
                    std::cout << "\t\t[inside task .. thread " << std::this_thread::get_id() << "]" << std::endl;
                }
            }});
        }
        if (true) {  // bind version
            int a, b;
            std::function<void()> task = std::bind(bindableFunction, a, b, &(tp.mtxCout));
            tp.pushTask(task);
        }
    }
}