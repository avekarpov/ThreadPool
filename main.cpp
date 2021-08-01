#include <iostream>
#include "ThreadPool.h"

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
