#include <thread>
#include <iostream>
#include <vector>
#include <queue>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <atomic>

class ThreadPool
{
    /*
    TP_NOT_READY     : not ready
    TP_POPULATING    : threads populating
    TP_READY         : threads populated = ready = available count 0 ~ N = tasking
    TP_TASK_PUSHED_ALL   : task pushed all = no more push
    TP_NEED_TERMINATE    : threads need to be terminated = specify user's purpose
    TP_TASK_ASSIGNED_ALL : task assigned all = can terminate threads after task done
    TP_TASK_DONE     : task done = thread terminatable = statistics or do what you want before termination if you need
    TP_TERMINATING   : thread terminating
    TP_TERMINATED    : thread terminated
    */
    enum tp_state : int { TP_NOT_READY, TP_POPULATING, TP_READY, TP_TASK_PUSHED_ALL, TP_NEED_TERMINATE, TP_TASK_ASSIGNED_ALL, TP_TASK_DONE, TP_TERMINATING, TP_TERMINATED };
    typedef std::function<void()> Task;
private:
    int threadCount = 0;
    tp_state state = TP_NOT_READY;


    std::condition_variable cvThreadPoolWait;
    std::vector<std::thread> workers;
    std::atomic<int> availableWorkerCount { 0 };
    std::atomic<int> terminatedWorkerCount { 0 };

    std::mutex mtxTaskCount;
    std::queue<Task> tasks;

    std::mutex mtxTaskAssign;
    std::condition_variable cvTaskAssign;

    std::mutex mtxThreadRelease;
    std::condition_variable cvThreadRelease;

    void populateThreadPool(int threadCount)
    {
        this->threadCount = threadCount;
        this->state = TP_POPULATING;
        for (int i = 0; i < threadCount; ++i) 
        {
            std::thread thread([this]() {
                {
                    std::lock_guard<std::mutex> lg(this->mtxCout);
                    std::cout << "\tthread " << std::this_thread::get_id() <<" starting..." << std::endl;
                }
                while(this->state <= TP_TASK_ASSIGNED_ALL)
                {
                    std::mutex mtxThreadPoolWait;
                    std::unique_lock<std::mutex> ul(mtxThreadPoolWait);
                    {
                        std::lock_guard<std::mutex> lg(this->mtxCout);
                        std::cout << "\tthread "  << std::this_thread::get_id() << " waiting..." << std::endl;
                    }
                    this->availableWorkerCount.fetch_add(1);
                    this->cvThreadRelease.notify_one();
                    this->cvThreadPoolWait.wait(ul , [this]{ return !this->tasks.empty() || this->state == TP_TERMINATING; }); // task exist, or in thread terminating step, it is valid wake up
                    this->availableWorkerCount.fetch_sub(1);
                    {
                        std::lock_guard<std::mutex> lg(this->mtxCout);
                        std::cout << "\tthread "  << std::this_thread::get_id() << " awaken !"<< std::endl;
                    }
                    while(!this->tasks.empty()) {
                        Task task;
                        {
                            std::lock_guard<std::mutex> lg(this->mtxTaskCount);
                            if (this->tasks.empty()) continue;
                            task = this->tasks.front();
                            this->tasks.pop();
                            this->cvTaskAssign.notify_one();
                        }
                        
                        {
                            std::lock_guard<std::mutex> lg(this->mtxCout);
                            std::cout << "\tthread "  << std::this_thread::get_id() << " tasking..." << std::endl;
                        }
                        task();
                        {
                            std::lock_guard<std::mutex> lg(this->mtxCout);
                            std::cout << "\tthread "  << std::this_thread::get_id() << " task (done)" << std::endl;
                        }
                    }
                }
                {
                    std::lock_guard<std::mutex> lg(this->mtxCout);
                    std::cout << "\tthread " << std::this_thread::get_id() << " terminated" << std::endl;
                }
                this->terminatedWorkerCount.fetch_add(1);
                this->cvThreadRelease.notify_one();
            });
            thread.detach();
            {
                std::lock_guard<std::mutex> lg(this->mtxCout);
                std::cout << "thread " << i + 1 << " generated & detached" << std::endl;
            }
        }
        this->state = TP_READY;
    }

    void noMoreTask()
    {
        this->state = TP_TASK_PUSHED_ALL;
    }

    void releaseWorkers()
    {
        if (this->state != TP_TASK_PUSHED_ALL) return;
        {
            std::lock_guard<std::mutex> lg(this->mtxCout);
            std::cout << "releaseWorkers()... " << std::endl;
        }
        this->state = TP_NEED_TERMINATE;

        std::unique_lock<std::mutex> ul1(this->mtxTaskAssign);
        if (!tasks.empty()) cvTaskAssign.wait(ul1, [this] { return tasks.empty(); }); // awake when all tasks popped(assigned to thread)
        this->state = TP_TASK_ASSIGNED_ALL;
        {
            std::lock_guard<std::mutex> lg(this->mtxCout);
            std::cout << "all task assigned" << std::endl;
        }
        std::unique_lock<std::mutex> ul2(this->mtxThreadRelease);
        if (this->availableWorkerCount.load() != this->threadCount) cvThreadRelease.wait(ul2, [this] { return this->availableWorkerCount.load() == this->threadCount; }); // awake when all task done (terminatable)
        this->state = TP_TASK_DONE;
        this->state = TP_TERMINATING;
        cvThreadPoolWait.notify_all(); // notify all threads to terminate
        {
            std::lock_guard<std::mutex> lg(this->mtxCout);
            std::cout << "all task done" << std::endl;
        }
        if (this->terminatedWorkerCount.load() != this->threadCount) cvThreadRelease.wait(ul2, [this] { return this->terminatedWorkerCount.load() == this->threadCount; }); // awake when all thread terminated
        this->state = TP_TERMINATED;
        {
            std::lock_guard<std::mutex> lg(this->mtxCout);
            std::cout << "all thread terminated" << std::endl;
        }
    }

public:
    std::mutex mtxCout;

    explicit ThreadPool(int maxThreadCount = 4) : threadCount(maxThreadCount) { populateThreadPool(this->threadCount); }
    ThreadPool(const ThreadPool& rhs) = delete;
    ThreadPool(ThreadPool&& rhs) = delete;
    ~ThreadPool() { noMoreTask(); releaseWorkers(); }

    void pushTask(const Task& task)
    {
        tasks.push(task);
        cvThreadPoolWait.notify_one();
        {
            std::lock_guard<std::mutex> lg(this->mtxCout);
            std::cout << "\t\t\ttask pushed" << std::endl;
        }
    }

    
    

};