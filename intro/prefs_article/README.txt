prefs_article: README.txt
Last modified: 11/9/98

To build and run this code:
1. Go to the directory prefslib/ and build prefslib.so by either running make from the command line or building one of the BeIDE projects.
2. Copy libprefs.so to sample/lib. This is the place that the sample application will look for prefslib.so when it builds and launches.
3. Go to the directory sample/ and build sample, again by either running make from the command line or building one of the BeIDE projects.
4. Run sample from the command line.

Note that sample will only find prefslib.so if the application is in the same place as the lib/ directory.