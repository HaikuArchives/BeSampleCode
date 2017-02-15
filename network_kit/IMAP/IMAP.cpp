//	IMAP.cpp
//	--------
//	Implementation of the IMAP protocol.
//
//	Copyright 1999, Be Incorporated.   All Rights Reserved.
//	This file may be used under the terms of the Be Sample Code License.

#include "IMAP.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

/*
 * Definitions for the TELNET protocol. Snarfed from the FTPClient Object,
 * which says they were snarfed from the BSD source.
 */
#define IAC     255
#define DONT    254
#define DO      253
#define WONT    252
#define WILL    251
#define xEOF    236

const uint32 kSmallBuffSize = 256; /* this just holds a line of text right now */

IMAPFlag::IMAPFlag(const char *name, bool permanent) :
	fName(name),
	fPermanent(permanent)
{
}

/*********************/

IMAPMailMessage::IMAPMailMessage(uint32 uid) :
	fUID(uid)
{
}

uint32 
IMAPMailMessage::UID() const
{
	return(fUID);
}

void 
IMAPMailMessage::PrintToStream()
{
}

/********************/

IMAPMailBox::IMAPMailBox(const char *name) :
fName(name),
fUnseen(0)
{
}

IMAPMailBox::~IMAPMailBox()
{
	/* clean up this list so we don't leak */
	while(fMessageList.CountItems() > 0) {
		delete (IMAPMailMessage *) fMessageList.ItemAt(0);
		fMessageList.RemoveItem((long int) 0);
	}

}

const char
*IMAPMailBox::Name()
{
	return(fName.String());
}

void 
IMAPMailBox::AddFlag(const char *flag)
{
	fFlagList.AddItem(new IMAPFlag(flag, true)); /* for now, we act like they are all permanent */
}

void 
IMAPMailBox::AddMessage(IMAPMailMessage *msg)
{
	if (msg) {
		fMessageList.AddItem(msg);
	}
}

IMAPMailMessage *
IMAPMailBox::FindMessage(uint32 uid)
{
	bool found=false;
	IMAPMailMessage	*tMsg=NULL;
	
	for(int i=0; i < fMessageList.CountItems() && found == false; i++) {
		tMsg = (IMAPMailMessage *) fMessageList.ItemAt(i);
		if (tMsg->UID() == uid) found = true;
	}

	if(found == true) {
		return (tMsg);
	} else {
		return (NULL);
	}		
}


/********************/

void 
IMAPMailBox::SetValidity(uint32 validity)
{
	fUIDValidity = validity;
}

void 
IMAPMailBox::SetNextUID(uint32 uid)
{
	fNextUID = uid;
}

void 
IMAPMailBox::SetUnseen(uint32 unseen)
{
	fUnseen = unseen;
}

void 
IMAPMailBox::PrintToStream()
{
	printf("  Mailbox: %s\n", fName.String());
	printf("    Validity: %ld\n", fUIDValidity);
	printf("    NextUID:  %ld\n", fNextUID);
	printf("    Unseen:   %ld\n", fUnseen);

	printf("    Message list: ");
	IMAPMailMessage	*tMsg;
	for(int i=0; i < fMessageList.CountItems(); i++) {
		tMsg = (IMAPMailMessage *) fMessageList.ItemAt(i);
		printf("%ld ", tMsg->UID());
	}
	printf ("\n");
}

/*************/

IMAPConnection::IMAPConnection(const char *hostname, const char *username, const char *password) :
	fSelectedMailBox(NULL),
	fUsername(username),
	fPassword(password),
	fAddress(hostname, 143),
	fEndpoint(NULL)
{
}

IMAPConnection::~IMAPConnection()
{
	if (fEndpoint) {
		SendCommand("LOGOUT");
		delete(fEndpoint);
	}

	/* clean up this list so we don't leak */
	while(fMailBoxes.CountItems() > 0) {
		delete (IMAPMailBox *) fMailBoxes.ItemAt(0);
		fMailBoxes.RemoveItem((long int) 0);
	}
}

bool 
IMAPConnection::Connect()
{
	fEndpoint = new BNetEndpoint();
	if (B_OK != fEndpoint->InitCheck()) {
		printf("InitCheck Failed\n");
		return false;
	}
	
	if(B_OK != fEndpoint->Connect(fAddress)) {
		printf("Failure.\n");
		return false;
	} else {
		printf("Connected\n");

		char	buffer[kSmallBuffSize];
		GetLine(buffer, kSmallBuffSize); /* odd case: the hello message is an untagged OK -- eat it here */		
		
		BString	login;
		login << "LOGIN " << fUsername << " " << fPassword;
		if(SendCommand(login.String())) {
			fConnected = true;
		} else {
			return false;
		}
		
		BString capability;
		capability << "CAPABILITY";
		SendCommand(capability.String());

		BString list;
		list << "LIST \"\" *";
		SendCommand(list.String());

	}
	return true;
}

const BList *
IMAPConnection::MailBoxes() const
{
	return &fMailBoxes;
}

bool 
IMAPConnection::SendCommand(const char *command)
{
	/* static in member function -- like a member variable that */
	/* only works in this function.								*/
	static int32 command_counter=0;

	BString	message("KRIMEZ");
	message << command_counter << " ";
	command_counter++;

	message << command << "\n";

	//Uncomment this to see the outgoing traffice
	//printf("Send: %s\n", message.String());

	fEndpoint->Send(message.String(), message.Length());
	return HandleResponse();
}

/* this is the root of all parsing that happens to incoming  */
/* data.  It gets ugly.  This is not entirely my fault.      */
/* To see why it isn't my fault, read RFC 2060.              */
/* The server is pretty much free to hand us any response it */
/* wants at any time--so we really cannot select our parsing */
/* based on the command we sent--we must be ready to         */
/* to correlate the server's response with our idea of the   */
/* connection's state.                                       */

/* another thing to notice is the extensive use of strtok(), */
/* which is not thread-safe.  Implementation of a PString    */
/* (a parse-string) class is left as an exercise for the     */
/* reader.                                                   */

bool 
IMAPConnection::HandleResponse()
{
	bool	success=true; /* optimistic */
	bool	done=false;
	char	buffer[kSmallBuffSize];

	memset(buffer, 0, kSmallBuffSize);

	while(done == false) {
		GetLine(buffer, kSmallBuffSize);

		/* Handle the basic parsing */
		char *tag = strtok(buffer, " ");
		char *response = strtok(NULL, " ");
		char *remainder = strtok(NULL, "\n");

		/* something blew up */
		if (strcmp(response, "BAD") == 0) {
			printf("BAD: %s %s %s\n", tag, response, remainder);
			return false;
		}

		/* here we are at the meaty bits--dealing with the tag / response pairs */
		if (strcmp(tag, "*") == 0) {
			/* the response was untagged */
			if (strcmp(response, "OK") == 0) {
				if (*remainder == '[') {
					remainder++;
					char *cruft = strtok(remainder, "]");
					
					char *subtag = strtok(cruft, " ");
					char *value = strtok(NULL, " \n");
					if (strcmp(subtag, "UIDVALIDITY") == 0) {
						if (fSelectedMailBox) {
							fSelectedMailBox->SetValidity(atol(value));
						}
					} else
					if (strcmp(subtag, "UIDNEXT") == 0) {
						if (fSelectedMailBox) {
							fSelectedMailBox->SetNextUID(atol(value));
						}
					} else
					if (strcmp(subtag, "UNSEEN") == 0) {
						if (fSelectedMailBox) {
							fSelectedMailBox->SetUnseen(atol(value));
						}
					} else
					if (strcmp(subtag, "PERMANENTFLAGS") == 0) {
						// we don't handle permanent/vs temporary flags at the moment
					} else {
						/* we complain about this--it usually means something unimplemented */
						printf("Unidentified subtag: %s Value: %s\n", subtag, value);
					}
				} else {
					printf("Untagged OK: %s\n", remainder);
					/* not sure why we would get an untagged ok...but we do... */
				}
			} else
			/* interesting IMAP tidbit.  Sometimes the response code has a number before it... */
			if (strcmp(remainder, "EXISTS") == 0) {
				fTotalMessages = atol(response);
			} else
			if (strcmp(remainder, "RECENT") == 0) {
				fRecentMessages = atol(response);
			} else
			/* ...but usually it does not. */
			if (strcmp(response, "CAPABILITY") == 0) {
				fCapabilities.SetTo(remainder);
			} else
			if (strcmp(response, "LIST") == 0) {
				HandleMailBox(remainder);
			} else
			if (strcmp(response, "FLAGS") == 0) {
				//printf("Selected Mailbox has flags?\n");
			} else
			if (strcmp(response, "NO") == 0) {
				//printf("Server said no.  Mailbox: %s  Reason: %s\n", fSelectedMailBox->Name().String(), remainder);
				printf("Server said no.  Reason: %s\n", remainder);
			} else
			if (strcmp(response, "SEARCH") == 0) {
				/* for now we assume that we get a list of all messages every time */
				if (fSelectedMailBox) {
					char *value = strtok(remainder, " \n");
					while (value) {
						if (fSelectedMailBox->FindMessage(atol(value)) == NULL) {
							fSelectedMailBox->AddMessage(new IMAPMailMessage(atol(value)));
						}
						value = strtok(NULL, " \n");
						/* do something */
					}
				} else {
					printf("ERROR? Results of Search, but no valid selected mailbox...\n");
				}
			} else
			if (strcmp(response, "BYE") == 0) {
				//parting is such sweet sorrow
				fEndpoint->Close();
				fConnected = false;
				done = true;
			} else {
				/* once again, check for something strange */
				printf("unknown untagged response: \"%s\"  %s\n", response, remainder);
			}

		/* handle the tagged responses */
		} else {
			/* every transaction ends with a tagged response */
			if (strcmp(response, "OK") == 0) {
				done=true;
			} else {
				printf("Tagged: %s %s\n", response, remainder);
			}
		}
	}
	return success;
}

bool 
IMAPConnection::SelectMailBox(IMAPMailBox *box)
{
	fSelectedMailBox = box;

	if (box) {
		BString command("SELECT \"");
	
		command << box->Name() << "\"";
		SendCommand(command.String());
	}
	return true; /* assume success for now */
}

IMAPMailBox* 
IMAPConnection::FindMailBox(const char *name)
{
	bool found=false;
	IMAPMailBox	*tBox=NULL;
	
	/* this is a linear search.  It works, but gets slower with more boxes */
	for(int i=0; i < fMailBoxes.CountItems() && found == false; i++) {
		tBox = (IMAPMailBox *) fMailBoxes.ItemAt(i);
		if (strcmp(tBox->Name(),name) == 0) found = true;
	}

	if(found == true) {
		return (tBox);
	} else {
		return (NULL);
	}		
}

void 
IMAPConnection::HandleMailBox(char *remainder)
{
	char *flags = strtok(remainder, ")");
	char *name = strtok(NULL, "\n{");

	/* I should probably stop complaining about skanky pointer math after this one */
	/* if the name starts with a ", bump the pointer past it and rub out the final " */
	if (*name == '"') {
		name++;
		name[strlen(name)-1] = '\0';
	}
	
	IMAPMailBox	*tBox;
	
	if (FindMailBox(name) == NULL)  {
		tBox = new IMAPMailBox(name);
		
		flags++; /* strip off the leading ( */
		char *flag = strtok(flags, " ");
		while(flag) {
			tBox->AddFlag(flag);
			flag = strtok(NULL, " ");
		}

		fMailBoxes.AddItem(new IMAPMailBox(name));
	}
}

size_t 
IMAPConnection::GetLine(char *buffer, size_t size)
{
	size_t count = 0;
	bool done = false;
	int c = '\0';
	
	while(done == false && count < size) {
		fEndpoint->Receive(&c, 1);
		if (c == EOF || c == xEOF || c == '\n') {
			done = true;
			buffer[count] = '\0';
		} else {
			if (c != '\r') {
				buffer[count] = (char) c;
				count++;
			}
		}
	}

// Uncomment this to see all the incoming traffic
//	printf("%s\n", buffer);
	return count;
}

void 
IMAPConnection::PrintToStream()
{
	printf("IMAPConnection::PrintToStream()\n");
	
	if (fConnected) {
		printf("Connected\n");
		printf("Capabilities: %s\n", fCapabilities.String());
		printf("Mailboxes:\n");
		IMAPMailBox *tBox;
		for (int i=0; i < fMailBoxes.CountItems(); i++) {
			tBox = (IMAPMailBox *) fMailBoxes.ItemAt(i);
			tBox->PrintToStream();
			if (tBox == fSelectedMailBox) {
				printf("<<SELECTED>>  Messages: %ld  Recent: %ld\n", fTotalMessages, fRecentMessages);
			}
		}
		
	} else {
		printf("Not Connected\n");
	}
}
