Big Bad Voodoo Teapot
by Mikol Ryon <ryon@be.com>

One of the questions  most asked often of DTS lately is,
"How do I make my OpenGL app use hardware acceleration?"
-- or some variation thereof. OK, here's how. If you're
running Genki 5 or 6, with your trusty Voodoo2 card installed,
along with the glide2 package from the optional folder of the
Genki CD, you're ready to get started.

I have a pair of simple examples here, which allow you
to test your setup and show you the basics of hardware
acceleration. To do this, I've taken the GLTeapot example code
straight off the Release 4 CD and made minimal changes,
accelerating from ~40 fps to ~200 fps. That's cool. You'll
find the code at<ftp://ftp.be.com/pub/samples/open_gl/GLTeapot-Voodoo.zip>.
All changes are labeled "voodoo" for easy viewing.

The two big alterations to GLTeapot are adding full screen
mode for hardware acceleration and we changing the resize mode
of our view from B_FOLLOW_ALL to B_FOLLOW_NONE. This is so that
our view resizes to the size of the window. With Voodoo hardware,
we must be in 800x600 or less to get hardware acceleration.
You'll see that most of the code changes simply involve my adding
the fancy Alt-F switch and menu option to go from Window to
FullScreen mode and back.

Another quick example is converting Michael Morrisey's 3DLife
<http://www.be.com/aboutbe/benewsletter/volume_II/Issue34.html>
to make use of Voodoo hardware. The code for this is at
<ftp://ftp.be.com/pub/samples/open_gl/3Dlife-Voodoo.zip>. The changes here
were a bit more extensive, but they're still relatively minor.
In lifeView.cpp, we again change the resize mode of our view to
B_FOLLOW_NONE. In lifeWin.cpp we change our main window to a
BDirectWindow, and enable DirectMode with mv->EnableDirectMode(true);
Also in lifeWin.cpp we implement lifeWin::DirectConnected and pass
them along to our view's implementation of DirectConnected.

The only other changes are adding the menu option and hot switch
to switch between FullScreen and Window mode.

This has been a short article, but I hope it answers one of DTS's
most frequently asked questions.
