#include "timer.h"

// --------------------------------------------------------------------------------------------------------------------------

cTimer::cTimer()
    : m_startTime(Clock::now()),
    m_previousTime(Clock::now()),
    m_deltaTime(0.0),
    m_isRunning(false),
    m_isPaused(false),
    m_pausedDuration(std::chrono::duration<double>(0))
{
}

// --------------------------------------------------------------------------------------------------------------------------

void cTimer::Start()
{
    if (!m_isRunning) 
    {
        if (m_isPaused) 
        {
            m_startTime = Clock::now() - std::chrono::duration_cast<std::chrono::steady_clock::duration>(m_pausedDuration);
            m_isPaused = false;
        }
        else 
        {
            m_startTime = Clock::now();
        }

        m_previousTime = m_startTime; 
        m_isRunning = true;            
    }
}

// --------------------------------------------------------------------------------------------------------------------------

void cTimer::Stop()
{
    if (m_isRunning) {
        m_deltaTime         = std::chrono::duration<double>(Clock::now() - m_previousTime).count();
        m_pausedDuration    = Clock::now() - m_startTime;
        m_isRunning         = false; 
        m_isPaused          = true;    
    }
}

// --------------------------------------------------------------------------------------------------------------------------

void cTimer::Tick()
{
    if (m_isRunning) 
    {
        TimePoint currentTime   = Clock::now();
        m_deltaTime             = std::chrono::duration<double>(currentTime - m_previousTime).count();
        m_previousTime          = currentTime;
    }
}

// --------------------------------------------------------------------------------------------------------------------------

double cTimer::GetTotalTime() const
{
    if (m_isRunning) 
    {
        return std::chrono::duration<double>(Clock::now() - m_startTime).count();
    }

    return std::chrono::duration<double>(m_pausedDuration).count();
}

// --------------------------------------------------------------------------------------------------------------------------

double cTimer::GetDeltaTime() const
{
    return m_deltaTime;
}

// --------------------------------------------------------------------------------------------------------------------------

void cTimer::Reset()
{
    m_startTime     = Clock::now();
    m_previousTime  = m_startTime;
    m_deltaTime     = 0.0;
    
    if (!m_isRunning) 
    {
        m_isPaused = false; 
    }
}

// --------------------------------------------------------------------------------------------------------------------------
