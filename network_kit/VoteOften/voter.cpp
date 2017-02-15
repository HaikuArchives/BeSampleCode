/* voter.cpp */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Locker.h>
#include <kernel/OS.h>
#include <stdio.h>
#include <socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <Autolock.h>

#include "voter.h"

voter::voter()
	: 	fThread(-1), fArrested(false), fVoting(false),
		fVotes(0), fServer(NULL), fMethod(NULL),
		fRequest(NULL), fAddr(0), fBufferSize(1024)
{
}

voter::~voter()
{
	//be sure to lock the thread before doing this
	bool locked = fLock.Lock();	
	
	if (locked) {
	
		//if we're being deleted and we're still voting
		if (fArrested == false) {
			Arrest();
		}
		
		delete [] fServer;
		delete [] fMethod;
		delete [] fRequest;
		fLock.Unlock();
	}
}

bool 
voter::Bribe(	const char *name, const char *server,
				const char *method, const char *request)
{
	//we need all of this info
	if ( name == NULL || server == NULL || method == NULL || request == NULL)
		return false;

	//if we are already set up and running don't do it again
	if (IsValid()) return false;

	if (SetTarget(server, method, request) == false)
		return false;
		
	fThread = spawn_thread(BribeMe, name, B_LOW_PRIORITY, this);
	if (fThread > 0) {
		return true;
	} else {
		printf("%s: thread not spawned\n", name);
		return false;
	}
}


//static thread entry function
int32 
voter::BribeMe(void *arg)
{
	voter *for_sale = (voter *)arg;
	return for_sale->Bribed();
}

int32 
voter::Bribed()
{
	//until we are arrested, we should do our thing
	while (!fArrested) {
		//if we are voting, lets vote already
		if (fVoting) {
			
			//lock while building connection info
			if(fLock.Lock()) {
				//figure out what we are voting for
				struct sockaddr_in address;
				address.sin_family = AF_INET;
				address.sin_port = htons(80);
				address.sin_addr.s_addr = fAddr;
				memset(&address.sin_zero, '0', sizeof(address.sin_zero));

				//build the request
				/* method + " " + request + "\r\nHost: " + host + " \r\nConnection: close\r\n\r\n" */
				int32 len = strlen(fMethod) + 1 + strlen(fRequest) + 8 + strlen(fServer) + 23;
			
				char *to_send = new char[len +1];
				sprintf(to_send, "%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n", fMethod, fRequest, fServer);
				
				//it's okay to unlock now
				fLock.Unlock();
			
				//lets get a socket
				int sock = socket(AF_INET, SOCK_STREAM, 0);
				//if we didn't get a socket let's clean up and then take a break
				if(sock < 0) {
					delete [] to_send;
					break;
				}
				
				//connect to our address
				long result = connect(sock, (struct sockaddr *)&address, sizeof(address));
				//if we can't connect let's clean up and then take a break
				if (result < 0) {
					closesocket(sock);
					delete [] to_send;
					break;
				}
				
				//let's actually vote
				ssize_t sent = _send_vote(sock, to_send, len);
				//if we could not send everything let's clean up and take a break
				if (sent != len) {
					printf("something wrong! sent: %ld\n", sent);
					closesocket(sock);
					delete [] to_send;
					break;
				}
				
				fVotes++;
				
				//let's get the results of our vote
				
				//we don't want to lock the entire time we are fetching
				//so we'll get the current value and use it throughout
				ssize_t buffer_size = fBufferSize;
			
				char * result_buf = new char[buffer_size];
				ssize_t count = buffer_size;
				ssize_t rcvd = 0;
				
				//keep going back to get more while the buffer returns filled
				while (count == buffer_size) {
					//we don't really care what the info is so we'll clear it out
					memset(result_buf, '0', buffer_size);
					count = _recv_result(sock, result_buf, buffer_size);
					if (count > 0) {
						rcvd += count;
					}
				}
				if (count < 0) printf("we have a read error!\n");		
				else printf("we rcvd %ld bytes back\n", rcvd);
			
				//in any case we want to clean up here
				delete [] result_buf;
				closesocket(sock);
				delete [] to_send;	
			}
		}
		//as long as we aren't voting let's take a nap
		if (!fVoting) {
			suspend_thread(fThread);
		}
		
	}

	//yikes! we've been arrested!
	return 0;
}

bool 
voter::Vote()
{
	//if we are not valid tell them
	if (!IsValid()) {
		printf("invalid\n");
		return false;	
	}
	
	
	//if we are already voting tell them
	if (fVoting) {
		printf("We are already voting!\n");
		return true;
	}
	bool locked = fLock.Lock();

	if (locked) {
		fVoting = true;
	
		//Wake Up!
		bool voting = false;
		if (resume_thread(fThread) == B_OK) voting = true;
		fLock.Unlock();
		return voting; 
	} else return false;	
}

bool 
voter::Cease()
{
	//if we are not valid tell them
	if (!IsValid()) {
		printf("invalid\n");
		return false;
	}
	
	bool locked = fLock.Lock();
	if (locked) {
		fVoting = false;
		fLock.Unlock();
		//the thread will suspend itself when it sees
		//it is not supposed to vote (ie after it's current vote)
		return true;
	}
	else return false;
}

bool 
voter::Arrest()
{
	
	if (!IsValid()) return false;
	
	//lock the voter
	bool locked = fLock.Lock();
	if (locked) {
		//arrest us
		fArrested = true;
		
		//wait for the thread to exit before returning
		//this will wake any suspended threads
		int32 result;
		bool arrested = false;
		if (wait_for_thread(fThread, &result) == B_OK)
				arrested = true;
		fLock.Unlock();
		return arrested;
		
	} else return false;
}

bool 
voter::SetTarget(	const char *server,
					const char *method,
					const char *request)
{

	//we need valid information
	if ( server == NULL || method == NULL || request == NULL)
		return false;


	//lock the voter
	bool locked = fLock.Lock();
	
	if (locked) {
		
		int32 len = 0;
		
		//if fServer is changing
		if (fServer == NULL || strcmp(fServer, server)) {
			if (fServer) delete [] fServer;
			len = strlen(server);
			fServer = new char[len +1];
			strcpy(fServer, server);
		}

		//if fMethod is changing
		if (fMethod == NULL || strcmp(fMethod, method)) {
			if (fMethod) delete [] fMethod;
			len = strlen(method);
			fMethod = new char[len +1];
			strcpy(fMethod, method);
		}

		//if fServer is changing
		if (fRequest== NULL || strcmp(fRequest, request)) {
			if (fRequest) delete [] fRequest;
			len = strlen(request);
			fRequest = new char[len +1];
			strcpy(fRequest, request);
		}
		
		//get the new target
		hostent *host =	gethostbyname(server);
		if (host == NULL) return false;
				
		fAddr = *(uint32 *)host->h_addr;
		
		fLock.Unlock();
		return true;
	} else return false;
}

//buffer size is restricted to between 1k and 8k
bool 
voter::SetBufferSize(uint32 bytes)
{
	if (bytes < 1024 || bytes > 8192) return false;
	
	bool locked =  fLock.Lock();
	
	if (locked) {
		fBufferSize = bytes;
		fLock.Unlock();
		return true;
	} else return false;
}

const bool 
voter::Voting() const
{
	return fVoting;
}

const bool 
voter::Arrested() const
{
	return fArrested;
}

const uint32 
voter::Votes() const
{
	return fVotes;
}

const char *
voter::Server() const
{
	return fServer;
}

const char *
voter::Method() const
{
	return fMethod;
}

const char *
voter::Request() const
{
	return fRequest;
}

const bool 
voter::IsValid()
{
	bool valid = false;
	bool locked = fLock.Lock();
	if (locked) {
		valid =  (fServer && fMethod && fRequest && (fThread > 0) && (fAddr != 0));
		fLock.Unlock();
	}
	return valid;
}

const ssize_t 
voter::BufferSize() const
{
	return fBufferSize;
}

ssize_t 
voter::_send_vote(int sock, char *buf, int buf_size)
{
	int bytes_sent = 0;
	int bytes_left = buf_size;
	int block_size = 1024;
	int num_bytes = 1;
	
	while( bytes_sent < buf_size && num_bytes > 0) {
		if (bytes_left < block_size) {
			num_bytes = send(sock, buf + bytes_sent, bytes_left, 0);
		} else {
			num_bytes = send(sock, buf + bytes_sent, block_size, 0);
		}
		if (num_bytes > 0) {
			bytes_sent += num_bytes;
			bytes_left -= num_bytes;
		}
	}
	if (num_bytes < 0) return errno;
	else return bytes_sent;
}

ssize_t 
voter::_recv_result(int sock, char *buf, const ssize_t buf_size)
{
	ssize_t bytes_rcvd = 0;
	ssize_t num_bytes = 1;
	
	// note that this will block until either the entire amount of data 
	// is rcvd or until the connection is terminated (when it returns 0)
	// this is fine with http 1.0 connections without keep alive, but not others
	while (bytes_rcvd < buf_size && num_bytes > 0) {
		num_bytes = recv(sock, buf + bytes_rcvd, buf_size - bytes_rcvd, 0);
		if (num_bytes > 0) bytes_rcvd += num_bytes;
	}
	
	if (num_bytes < 0) return errno;
	else return bytes_rcvd;
}
