#include <chrono>

using Clock     = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

class cTimer {
    public:
        
        cTimer();

    public:

        void Start();
        void Stop();
        void Tick();
        
        double GetTotalTime() const;
        double GetDeltaTime() const;
        
        void Reset();

    private:

        TimePoint m_startTime;  
        TimePoint m_previousTime;

        double m_deltaTime;

        bool m_isRunning;
        bool m_isPaused;                

        std::chrono::duration<double> m_pausedDuration;  
};