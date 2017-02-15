Doodle -- README (last edited 11/9/98)
Owen Smith, DTS Engineer, Be, Inc.

This simple sketching app demonstrates how a Be application can be constructed.
It is similar to the Microsoft Foundation Class tutorial "Scribble." Comments
labeled "MFC NOTE" indicate particular comparisons between the Be approach and
the MFC approach.

Two Be Newsletter articles accompany this example:

article1.txt is the original newsletter article which this application was
written for. It outlines some of the significant differences between Windows
and Be programming.

article2.txt is a followup article which deals with the concept of modality and
synchronous dialog behavior. This code incorporates changes that were made to
Doodle to handle such synchronous behavior correctly.

This code has also been modified for R4 to take advantage of asynchronous mouse
handling in DudeView (more akin to the MFC approach), to improve drawing
slightly, and to fix a couple more bugs.
