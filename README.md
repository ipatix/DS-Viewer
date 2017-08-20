# DS-Viewer
This is the viewer application for my DS Video Capture Device

This is some dirty C++ code that can interface to the FT232H from my DS Video Capture device.

If you are crazy enough to go through the code and compile it, the master branch uses the proprieitary ftd2xx library which only seems to perform decent enough on Windows.
For Linux users use the libftdi branch. It uses a slightly more up to date codebase. 
Note that the performance will heavily depend on the scheduling of the USB thread on Linux (I don't understand enough libusb to make this nicer) and will require a nice of -20 to not loose data packets.

Dependencies are boost and libsdl2

A demo video can be found here:
https://www.youtube.com/watch?v=MxQT0u2nWGM
