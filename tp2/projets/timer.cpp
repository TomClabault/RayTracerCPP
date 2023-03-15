#include "timer.h"

void Timer::start()
{
    m_start = std::chrono::high_resolution_clock::now();
}

void Timer::stop()
{
    m_stop = std::chrono::high_resolution_clock::now();
}

long long int Timer::elapsed()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(m_stop - m_start).count();
}
