//	MailCrimez.cpp
//	--------------
//	A simple example that uses IMAP.
//
//	Copyright 1999, Be Incorporated.   All Rights Reserved.
//	This file may be used under the terms of the Be Sample Code License.

#include "IMAP.h"
#include <stdio.h>

const char	kUsername[] = "adam";
const char	kPassword[] = "runcible";

int main(void) {

	/* here we create a connection object */
	IMAPConnection	tConnection("mail.diamond-age.net",kUsername, kPassword);

	/* ask it to connect */
	tConnection.Connect();

	/* Now that we are connected, we iterate through the mailboxes and select */
	/* each one, which forces the server to give us uid/validity info */	
	IMAPMailBox	*tbox;
	for(int i=0; i < tConnection.MailBoxes()->CountItems(); i++) {
		tbox = (IMAPMailBox *) tConnection.MailBoxes()->ItemAt(i);
		tConnection.SelectMailBox(tbox);
	}

	/* to get an actual list of messages, we select a mailbox... */
	tConnection.SelectMailBox(tConnection.FindMailBox("INBOX"));

	/* ...and send a command to search and return all of the messages in the box */
	tConnection.SendCommand("SEARCH ALL");

	/* This gives us copious debugging information */
	tConnection.PrintToStream();
}
