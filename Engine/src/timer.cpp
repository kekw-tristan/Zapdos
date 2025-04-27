#include "timer.h"

#include <windows.h>

// ---------------------------------------------------------------------------------------------
// constructor

cTimer::cTimer()
    : m_secondsPerCount(0.)
    , m_deltaTime(-1.)
    , m_baseTime(0)
    , m_pausedTime(0)
    , m_stopTime(0)
    , m_previousTime(0)
    , m_currentTime(0)
    , m_isStopped(false)
{
    __int64 countsPerSec;
    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSec));
    m_secondsPerCount = 1.0 / static_cast<double>(countsPerSec);
    
}

// ---------------------------------------------------------------------------------------------
// returns game time in seconds

float cTimer::GetGameTime() const
{
    if (m_isStopped)
    {
        return static_cast<float>(((m_stopTime - m_pausedTime) - m_baseTime) * m_secondsPerCount);
    }
    else
    {
        return static_cast<float>(((m_currentTime - m_pausedTime) - m_baseTime) * m_secondsPerCount);
    }
}

// ---------------------------------------------------------------------------------------------
// returns delta time in seconds

float cTimer::GetDelatTime() const
{
    return static_cast<float>(m_deltaTime);
}

// ---------------------------------------------------------------------------------------------
// resets the timer

void cTimer::Reset()
{
    __int64 currTime;

    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

    m_baseTime        = currTime;
    m_previousTime    = currTime;
    m_stopTime        = 0;
    m_isStopped       = false;
}

// ---------------------------------------------------------------------------------------------
// unpauses the timer

void cTimer::Start()
{
    __int64 startTime;
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&startTime));

    if (m_isStopped)
    {
        m_pausedTime  += (startTime - m_stopTime);

        m_stopTime    = 0;
        m_isStopped   = false;
    }
}

// ---------------------------------------------------------------------------------------------
// pauses the timer

void cTimer::Stop()
{
    if (!m_isStopped)
    {
        __int64 currTime; 
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

        m_stopTime    = currTime;
        m_isStopped   = true;
    }
}

// ---------------------------------------------------------------------------------------------
// needs to be called every frame

void cTimer::Tick()
{
    if (m_isStopped)
    {
        m_deltaTime = 0.;
        return;
    }

    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_currentTime));

    m_deltaTime = (m_currentTime - m_previousTime) * m_secondsPerCount;

    m_previousTime = m_currentTime;

    if(m_deltaTime < 0)
    {
        m_deltaTime = 0.;
    }
}

// ---------------------------------------------------------------------------------------------