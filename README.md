# DS-Viewer
This is the viewer application for my DS Video Capture Device

This is some dirty C++ code that can interface to the FT232H from my DS Video Capture device.

The code should work on both Windows and Linux (Mac not tested, but could work).
Windows Users will require the proprietary FTD2XX library. Unix users have to use libftdi (which comes on most Linux distributions).
Note that the performance will heavily depend on the scheduling of the USB thread on Linux (I don't understand enough libusb to make this nicer) and will require a nice of -20 to not loose data packets.

I'm sorry there is not much documentation available. If you have any questions, open up a new issue and I will try to add information to this Readme and/or other places.

Dependencies are boost and libsdl2

A demo video can be found here:
https://www.youtube.com/watch?v=MxQT0u2nWGM
