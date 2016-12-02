#include <iostream>
#include <exception>
#include <chrono>

#include "MediaViewer.h"
#include "CableReceiver.h"
#include "uint128_t.h"
#include "Config.h"

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
        Image top(VIDEO_HEIGHT, VIDEO_WIDTH);
        Image bot(VIDEO_HEIGHT, VIDEO_WIDTH);
        MediaViewer mv(top, bot);
        //DummyReceiver receiver;
		DSReciever receiver;

		bool running = true;
		auto lastTime = chrono::high_resolution_clock::now();
        while (running && !receiver.HasStopped())
        {
            receiver.Receive(top, bot, mv.GetAudioBuffer());
            running = mv.UpdateVideo(false);

			auto afterTime = chrono::high_resolution_clock::now();
			//  cout << "Rendering took: " << chrono::duration_cast<chrono::milliseconds>(afterTime - lastTime).count() << " ms" << endl;
			lastTime = afterTime;
        }
		receiver.Stop();
		while (true)
		{
			//receiver.Receive(top, bot, mv.GetAudioBuffer());
			if (receiver.HasStopped())
				break;
		}
    }
    catch (const std::exception& e)
    {
        cerr << e.what() << endl;
		this_thread::sleep_for(chrono::milliseconds(1000));
    }

    return 0;
}
