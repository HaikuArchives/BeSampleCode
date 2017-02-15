//	IMAP.h
//	------
//	Basic classes that implement the IMAP protocol.
//
//	Copyright 1999, Be Incorporated.   All Rights Reserved.
//	This file may be used under the terms of the Be Sample Code License.

#ifndef IMAP_h
#define IMAP_h

#include <String.h>
#include <List.h>
#include <NetAddress.h>
#include <NetBuffer.h>
#include <NetEndpoint.h>
#include <NetDebug.h>

/* client commands -- Any State										*/
/* CAPABILITY		: returns list of server's capabilities 		*/
/* NOOP				: resets keepalive, may get buffered response	*/
/* LOGOUT			: Logs you out									*/

/* -- Non-Authenticated State										*/
/* AUTHENTICATE <method> : 	sets the authentication method for the	*/
/* 							session and is followed by a handshake	*/
/*						 	protocol								*/

/* LOGIN <username> <password> : logs in with given username and	*/
/*								password.							*/

/* -- Authenticated State											*/
/* SELECT <mailbox>			: selects mailbox for transactions		*/
/* CREATE <mailbox>			: creates the named mailbox				*/
/* DELETE <mailbox>			: deletes the named mailbox				*/
/* RENAME <from> <to>		: renames mailbox <from> to <to>		*/
/* SUBSCRIBE <mailbox>		: "subscribes" to a mailbox				*/
/* UNSUBSCRIBE <mailbox>	: "unsubscribes" from a mailbox			*/
/* LIST <refname> <list>	: Lists mailboxes						*/
/*								<refname> = mailbox or heirarchy	*/
/*								<list> = mailbox					*/
/* LSUB <refname> <list>	: Lists subscribed mailboxes			*/
/* STATUS <mailbox> <arg> 	: Status of mailbox						*/
/* 		arg==MESSAGES		: number of messages					*/
/* 		arg==RECENT			: number of \Recent messages			*/
/*		arg==UIDNEXT		: next UID for a message				*/
/*		arg==UIDVALIDITY	: unique ID validity value for box		*/
/*		arg==UNSEEN			: number of non \Seen messages			*/	

/* APPEND <mailbox> <flags> <date/time> [message]					*/
/*		Appends the supplied message to <mailbox>					*/

/* -- Selected State												*/
/* CHECK					: checkpoints selected mailbox			*/
/* CLOSE					: closes mailbox, deleting \Deleted		*/
/* EXPUNGE					: closes mailbox, deleting \Deleted		*/
/* SEARCH					: searches mailbox for messages			*/
/* FETCH <message set>		: fetches specified messages			*/
/*specials: ALL, BODY, HEADER, HEADER.FIELDS, HEADER.FIELDS.NOT, TEXT*/
/* STORE <message set> <data item> <data value>	: sets flags		*/
/* COPY <message set> <mailbox name>: copies messages				*/
/* UID						: prepended to COPY/FETCH/STORE/SEARCH	*/

class IMAPFlag {
public:
	IMAPFlag(const char *name, bool permanent);
private:
	BString		fName;
	bool		fPermanent;
};

class IMAPMailMessage {
public:
	IMAPMailMessage(uint32 uid);/* creates a new message with the given uid */

	uint32 UID() const;			/* returns the Unique ID of the message */

	void PrintToStream();
private:
	uint32		fUID;			/* Unique Identifier			*/
};

class IMAPMailBox {
public:
	IMAPMailBox(const char *name);
	~IMAPMailBox();
	
	const char *Name();
	
	void AddFlag(const char *flag);
	void AddMessage(IMAPMailMessage *msg);
	IMAPMailMessage* FindMessage(uint32 uid);
	
	void SetValidity(uint32 validity);
	void SetNextUID(uint32 uid);
	void SetUnseen(uint32 unseen);

	void PrintToStream();
private:
	BString		fName;

	BList		fMessageList;	/* BList of MAPIMailMessages		*/
	BList		fFlagList;		/* BList of MAPIFlags				*/
	
	uint32		fUIDValidity;	/* Unique Identifier Validity value	*/
	uint32		fNextUID;		/* the server's suggested next UID	*/
	uint32		fUnseen;		/* the number of unseen messages	*/
};	

class IMAPConnection {
public:
	IMAPConnection(const char *hostname, const char *username, const char *password);
	~IMAPConnection();

	bool SendCommand(const char *command);
	bool Connect();

	const BList *MailBoxes() const;

	IMAPMailBox* FindMailBox(const char *name);
	/* one should never call SelectMailBox() on an IMAPMailBox that isn't in the MailBoxes() list */
	bool SelectMailBox(IMAPMailBox *box);

	void PrintToStream();
private:
	size_t GetLine(char *buffer, size_t size);	/* fetch the next "line" of text					*/
	
	bool HandleResponse();						/* handle the response								*/
	void HandleMailBox(char *remainder);		/* handle mailbox responses							*/
	
	BList			fMailBoxes;					/* BList of MAPIMailBoxes							*/
	IMAPMailBox		*fSelectedMailBox;
	
	uint32			fRecentMessages;			/* the number of recent messages in selected box	*/
	uint32			fTotalMessages;				/* the number of total messages in selected box		*/

	bool			fConnected;					/* are we connected?								*/
	BString			fCapabilities;				/* list of the server's capabilities				*/

	BString			fUsername;					/* user name to login								*/
	BString			fPassword;					/* password for login								*/
	BNetAddress		fAddress;					/* address of remote server							*/
	BNetEndpoint	*fEndpoint;					/* actual connection to the server					*/
};
#endif