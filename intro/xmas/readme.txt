DEVELOPERS WORKSHOP: Memories of Tomorrow
By Mikol Ryon (ryon@be.com)

With BeOS R4 just out of the chute, I've prepared a
little number that shows off a few of its new features:

<ftp://ftp.be.com/pub/samples/intro/xmas.zip>

This is sort of a remake of an old Mac demo, updated
in R4 style.

The first new goodie is xres, a tool that lets you
manipulate resource files. With xres you can create
resource files, merge them with applications, and add
and remove resources from those files and applications.

In this example, we'll use xres to load an image into
a resource file for use in code. If you already have a
resource file, perhaps one that contains the apps icon,
this will add the image to the pre-existing resource
file:

xres -o xmas_x86.rsrc -a bits:666:xmas_bmap newr4.jpg \
xmas_x86_No_bitmap.rsrc

Here, xmas_x86.rsrc is the resource (if it doesn't exist,
it will be created) and bits:666:xmas_bitmap is the
type:ID:name of the resource you're adding. (You can name
these elements anything you want, but the name type 'bits'
is a good choice for an image, as you'll see in a minute.)
newr4.jpg and xmas_x86.rsrc are the two files being merged.

Now execute xres (you can type --help if you need more
info) and add your resource file to the project file, if
you haven't already done so.

Next we need to do something with our image:

#include <TranslationUtils.h>
#include <Bitmap.h>

...

BBitmap *icon_bitmap = BTranslationUtils::GetBitmap("xmas_bmap");

This takes you from .jpg file to BBitmap with just one command
and three lines of code. GetBitmap is nice because it looks
first for an image file with the name you gave it in the
applications folder. If the file isn't there, GetBitmap looks
in the resources file for a resource of type 'bits' and the
name you gave it.

The next code snippet takes a peek at the new BString class.
In it, we first declare an empty BString, then use + operations
to add characters to the string -- just as we would with
numerical variables. The BString class is very cool and will
surely have an entire article devoted to it in the future.

#include <String.h>
...

	BString path_string;
	path_string+=file_path.Path();
	path_string+="/xmas.wav";

	char snd_path[path_string.CountChars()+1];
	path_string.CopyInto(snd_path, 0, path_string.CountChars()+1);

Also new in R4 is using the Media Kit to play a familiar
sound track (thanks, Baron!). Take a look at Eric Shepherd's
recent article
<http://www.be.com/aboutbe/benewsletter/volume_II/Issue45.html#Workshop>
for more about sounding off with the new Media Kit.

Currently, BSoundPlayer assumes a 44kHz sound, which is why I
use this bit of code to tell the constructor to use an alternate
sample rate:

	media_raw_audio_format fmt = sound->Format();
	BSoundPlayer player(&fmt, ref.name);


That brings me to the end of my R4 teaser. My New Year's
resolution is to turn this article file into a generic reminder
daemon, so I can continue to bring you breaking news as it
happens (and I can talk about it without breaching my NDA).
