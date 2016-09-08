#include <iostream>
#include <exception>

#include "MediaViewer.h"
#include "CableReceiver.h"
#include "uint128_t.h"

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
        Image top(192, 256);
        Image bot(192, 256);
        MediaViewer mv(top, bot);
        DummyReceiver receiver;

		bool running = true;
        while (running)
        {
            receiver.Receive(top, bot, mv.GetAudioBuffer());
            running = mv.UpdateVideo(false);
        }
		receiver.Stop();
		while (true)
		{
			receiver.Receive(top, bot, mv.GetAudioBuffer());
			if (receiver.HasStopped())
				break;
		}
    }
    catch (const std::exception& e)
    {
        cerr << e.what() << endl;
    }

    return 0;
}
