#include <iostream>
#include <exception>
#include <chrono>

#include <GL/glew.h>

#include "MediaViewer.h"
#include "CableReceiver.h"
#include "Config.h"

// needed to make the nuget SDL2 version work
#ifdef _WIN32
extern "C" { FILE __iob_func[3] = {*stdin, *stdout, *stderr}; }
#endif

using namespace std;

int main(int argc, char **argv) 
{
	(void)argc;
	(void)argv;

    try
    {
		boost::lockfree::spsc_queue<stereo_sample> audio_buffer(AUDIO_BUF_SIZE);
        MediaViewer mv(audio_buffer);
        //DummyReceiver receiver;
		DSCableReceiver receiver(audio_buffer);

		bool running = true;
		auto lastTime = chrono::high_resolution_clock::now();
        while (running)
        {
            auto currentTime = chrono::high_resolution_clock::now();
            chrono::nanoseconds frameTime = currentTime - lastTime;
            lastTime = currentTime;
            running = mv.UpdateVideo(receiver, frameTime.count() / float(1000000000.0f));

            //printf("frame time: %f\n", frameTime.count() / float(1000000000.0f));
        }
    }
    catch (const std::exception& e)
    {
        cerr << e.what() << endl;
		this_thread::sleep_for(chrono::milliseconds(50000));
    }

    return 0;
}
