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
        Image top(NDS_H, NDS_W);
        Image bot(NDS_H, NDS_W);
		boost::lockfree::spsc_queue<float> audio_buffer(AUDIO_BUF_SIZE * 2);
        MediaViewer mv(top, bot, audio_buffer);
        //DummyReceiver receiver;
		DSReciever receiver(top, bot, audio_buffer);

		bool running = true;
		auto lastTime = chrono::high_resolution_clock::now();
        while (running)
        {
            receiver.Receive();
            running = mv.UpdateVideo(false);

			auto afterTime = chrono::high_resolution_clock::now();
			//  cout << "Rendering took: " << chrono::duration_cast<chrono::milliseconds>(afterTime - lastTime).count() << " ms" << endl;
			lastTime = afterTime;
        }
    }
    catch (const std::exception& e)
    {
        cerr << e.what() << endl;
		this_thread::sleep_for(chrono::milliseconds(50000));
    }

    return 0;
}
