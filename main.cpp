#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>
#include <future>

class ThreadPool
{
public:
    explicit ThreadPool(unsigned numberThreads = std::thread::hardware_concurrency()) : _isStop(false)
    {
        for(unsigned i = 0; i < numberThreads; ++i)
        {
            this->_workers.emplace_back([&]()
            {
                std::function<void()> task;

                while(!_isStop)
                {
                    {
                        std::unique_lock<std::mutex> lock(_tasksMutex);

                        if(_tasks.empty())
                        {
                            _waiterForNewTask.wait(lock);

                            if(_isStop || _tasks.empty())
                            {
                                break;
                            }
                        }

                        task = _tasks.front();
                        _tasks.pop();
                    }

                    task();
                }
            });
        }
    }

    template<typename F, typename ...Args>
    auto enqueue(F &&taskFunction, Args &&...args)->std::future<typename std::invoke_result<F, Args ...>::type>
    {
        using returnType = typename std::invoke_result<F, Args ...>::type;

        //auto task = std::make_shared<std::packaged_task<returnType()>>([&](){return taskFunction(args...);});
        auto task = std::make_shared<std::packaged_task<returnType()>>([&](){return taskFunction(args...);});
        auto future = (*task).get_future();
        //auto future = task.get_future();

        {
            std::unique_lock<std::mutex> lock(_tasksMutex);

            _tasks.push([=](){(*task)();});
        }

        _waiterForNewTask.notify_one();

        return future;
    }

    ~ThreadPool()
    {
        _isStop = true;

        _waiterForNewTask.notify_all();

        for(auto &worker : _workers)
        {
            worker.join();
        }
    }

private:
    std::vector<std::thread> _workers;

    std::queue<std::function<void()>> _tasks;
    std::mutex _tasksMutex;

    std::condition_variable _waiterForNewTask;

    bool _isStop;
};

char test(char c = '*')
{
    for(unsigned i = 0; i < 20; ++i)
    {
        std::cout << c;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return c;
}

int main()
{
    ThreadPool threadPool(2);

    std::vector<std::pair<std::function<char(char)>, char>> tasks{{test, '*'}, {test, '.'}, {test, '+'}, {test, '-'}, {test, '#'}};
    std::vector<std::future<char>> futures;

    futures.reserve(tasks.size());

    for(const auto &task : tasks)
    {
        futures.push_back(threadPool.enqueue(task.first, task.second));
    }

    futures.front().wait();

    for(auto &future : futures)
    {
        future.wait();
    }

    std::cout << std::endl;
    for(auto &future : futures)
    {
        std::cout << "[ " << future.get() << " ]" << std::endl;
    }

    return 0;
}
