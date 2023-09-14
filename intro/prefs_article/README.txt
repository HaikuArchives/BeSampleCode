prefs_article: README.txt
Last modified: 15/9/23

To build and run this code:
1. Go to the directory prefslib/ and build prefslib.so by running make from the command line.
2. Copy libprefs.so to sample/lib. This is the place that the sample application will look for prefslib.so when it builds and launches.
3. Go to the directory sample/ and build sample, again by either running make from the command line.
4. Run sample from the command line.

Note that sample will only find prefslib.so if the application is in the same place as the lib/ directory.