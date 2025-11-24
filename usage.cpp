#include <iostream>
#include "FpsCounter.h"
#include <thread>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

FpsCounter g_Fps;


void GameLoop()
{
    timeBeginPeriod(1);

    while (true)
    {
        if (g_Fps.Tick())
        {
            int fps = g_Fps.GetFps();
            if (fps > 0)
                printf("FPS: %d\n", fps);

        }

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    timeEndPeriod(1);
}

int main()
{
	GameLoop();
	return 0;
}