#pragma once

class cTimer
{
	public:

		cTimer(); 

	public:

		float GetGameTime() const;
		float GetDelatTime()const; 

	public:

		void Reset();
		void Start();
		void Stop(); 
		void Tick(); 

	private:

		double m_secondsPerCount;
		double m_deltaTime;

	private:

		__int64 m_baseTime;
		__int64 m_pausedTime;
		__int64 m_stopTime;
		__int64 m_previousTime;
		__int64 m_currentTime;

	private:

		bool m_isStopped;

};