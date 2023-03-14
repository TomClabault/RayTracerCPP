#include <chrono>

class Timer
{
public:
    void start();
    void stop();

    unsigned long int elapsed();

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_stop;
};
