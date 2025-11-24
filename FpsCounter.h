#include <chrono>

class FpsCounter
{
public:
    FpsCounter()
    {
        m_LastTime = std::chrono::steady_clock::now();
    }

    bool Tick()
    {
        ++m_FrameCount;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastTime);

        if (elapsed.count() >= 1000)
        {
            m_CurrentFps = m_FrameCount;

            m_FrameCount = 0;
            m_LastTime = now;

            return true;
        }

        return false;
    }

    int GetFps() const { return m_CurrentFps; }

private:
    std::chrono::steady_clock::time_point m_LastTime;
    int m_FrameCount = 0;
    int m_CurrentFps = 0;
};
