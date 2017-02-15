/* VoterApp.cpp */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "VoterApp.h"
#include "voter.h"
#include "VoterStats.h"

#include <List.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int32 MAX_VOTERS =	10;
const int32 START_VOTERS = 	0;

//places to go
const char * server = "silverlock.be.com";
const char * method = "GET";
const char * request = "/index.html";


VoterApp::VoterApp()
		: 	BApplication("application/x-vnd.BeDTS-VoteOften"),
			fVoting(false)
{
}

VoterApp::~VoterApp()
{
	//arrest all of the voters
	ArrestVoters(MAX_VOTERS);
}

void 
VoterApp::ReadyToRun()
{
	//http:// + server + request
	int32 len = 7 + strlen(server) + strlen(request) + 1;
	char *text = new char[len];
	sprintf(text, "http://%s%s", server, request);
	
	fStats = new VoterStats(text);
	fStats->Show();
	printf("text: %s\n", text);
	delete [] text;
	
	
	SetPulseRate(500000);
}

void 
VoterApp::ArgvReceived(int32 argc, char **argv)
{
	printf("argc: %ld\n", argc);
	if (argc == 1) {
		printf("VoteOften Syntax:\n"
				"=NUMBER: set the number of voters to NUMBER\n"
				"+NUMBER: bribe NUMBER additional voters\n"
				"-NUMBER: arrest NUMBER voters\n"
				"CEASE: stop voting\n"
				"STUFF: start voting\n"
		);
		return;
	}	
	
	for (int i = 1; i < argc; i++) {

		if (*(argv[i]) == '=') {
			if (strlen(argv[i]) > 1) {
				int32 num = atoi(argv[i] + 1);
				printf("num: %ld\n", num);
				//set the number of voters to num
				int32 diff = num - fPayroll.CountItems();
				if (diff > 0) {
					status_t status = BribeVoters(diff);
					if (status < 0) printf("problem bribing voters");
					else printf("%ld voters bribed\n", status);
				} else if (diff < 0) {
					status_t status = ArrestVoters(-diff);
					if (status < 0) printf("problem arresting %ld voters\n", status);
					else printf("%ld voters arrested\n", num);
				}
			} else {
				printf("syntax: =NUMBER expected\n");
			}
		}
		//bribe voters
		else if (*(argv[i]) == '+') {
			if (strlen(argv[i]) > 1) {
				int32 num = atoi(argv[i] + 1);
				printf("num: %ld\n", num);
				if (num > 0) {
					//let's hire num voters
					status_t status = BribeVoters(num);
					if (status < 0) printf("problem bribing voters");
					else printf("%ld voters bribed\n", status);
				}
			} else {
				printf("syntax: +NUMBER expected\n");
			}
		}
		//arrest voters
		else if (*(argv[i]) == '-') {
			if (strlen(argv[i]) > 1) {
				int32 num = atoi(argv[i] + 1);
				printf("num: %ld\n", num);
				if (num > 0) {
					//lets arrest num voters
					status_t status = ArrestVoters(num);
					if (status < 0) printf("problem arresting %ld voters\n", status);
					else printf("%ld voters arrested\n", num);
				}
			} else {
				printf("syntax: -NUMBER expected\n");
			}
		}		
		else if (!strcmp(argv[i], "CEASE")) {
			printf("CEASE found\n");
			BMessage msg(CEASE_AND_DESIST);
			fStats->PostMessage(&msg, fStats);
			CeaseAndDesist();
		}
		else if (!strcmp(argv[i], "STUFF")) {
			printf("STUFF found\n");
			BMessage msg(STUFF_THE_BALLOT);
			fStats->PostMessage(&msg, fStats);
			StuffTheBallot();	
		}

	}	
}

void 
VoterApp::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case HIRE_VOTERS:
		{
			int32 voters = 0;
			if (msg->FindInt32("voters", &voters) == B_OK && voters > 0)
				BribeVoters(voters);
			break;
		}
		
		case STUFF_THE_BALLOT:
		{
			printf("StuffTheBallot!\n");
			StuffTheBallot();
			break;
		}
		
		case CEASE_AND_DESIST:
		{
			printf("CeaseAndDesist!\n");
			CeaseAndDesist();
			break;
		}
		
		case ARREST_VOTERS:
		{
			int32 felons = 0;
			if (msg->FindInt32("voters", &felons) == B_OK)
				ArrestVoters(felons);
			break;
		}	
			
		default:
			BApplication::MessageReceived(msg);
	}
}

void 
VoterApp::Pulse()
{
	//step through each voter and get the vote count
	//note that this means that the vote count will only
	//be the number of votes that the given set of voters
	//have cast.  Having a total vote count is an exercise
	//left to the user
	uint64 votes = 0;
	
	int32 felons = fPayroll.CountItems();
	
	for (int i = 0; i < felons; i++) {
		voter * felon = (voter *) fPayroll.ItemAt(i);
		if (felon)
			votes += felon->Votes();
	}
	
	BMessage msg(UPDATE_VOTECOUNT);
	msg.AddInt64("votes", votes);
	msg.AddInt32("felons", felons);
	fStats->PostMessage(&msg);
}



status_t 
VoterApp::BribeVoters(int32 num)
{
	//make sure we don't have more than MAX_VOTERS
	int32 count = fPayroll.CountItems();
	int32 room = MAX_VOTERS - count;
	int32 bribes = 0;
	
	if (num > room) num = room;

	//are we going to bribe anybody?
	if (num <= 0) return B_ERROR;
	
	printf("bribing %ld more voters!\n", num);
	
	for (int i = 0; i < num; i++) {
		voter *felon = NULL;
		 felon = new voter();
		if (felon) {
			count = fPayroll.CountItems();
			char name[16];
			sprintf(name, "Voter_%ld", count + 1);
			if (!felon->Bribe(name, server, method, request))
				delete felon;
			
			bribes++;
			printf("bribed %s successfully!\n", name);
			
			if (fVoting) felon->Vote();
			
			fPayroll.AddItem(felon);
		}
	}

	return bribes;
}

status_t 
VoterApp::StuffTheBallot()
{
	if (fVoting) return B_ERROR;
	fVoting = true;
	
	fPayroll.DoForEach(stuff);
	return B_OK;
}

status_t 
VoterApp::CeaseAndDesist()
{
	if (!fVoting) return B_ERROR;
	fVoting = false;
	
	fPayroll.DoForEach(cease);
	return B_OK;
}

status_t 
VoterApp::ArrestVoters(int32 num)
{
	int32 left = num;
	int32 count = fPayroll.CountItems();
	if (left > count) left = count;
	if (left <= 0) return left;
	
	//while I still have felons left to arrest
	while (left > 0 && !fPayroll.IsEmpty()) {
		voter *felon = (voter *)fPayroll.RemoveItem((int32)0);
		if (felon) {
			if (felon->Arrest() == true) left--;
			delete felon;
		}
	}

	//left should be 0 at this point
	//if not then return negative the number
	//we couldn't arrest
	return -left;	
}

bool
cease(void *arg)
{
	voter *felon = (voter *) arg;
	if (felon->Cease() == true) printf("fewer votes\n");
	else printf("voting not ceased\n");
	return false;
}

bool
stuff(void *arg)
{
	voter *felon = (voter *) arg;
	if (felon->Vote() == true) printf("more votes\n");
	else printf("voting failed\n");
	return false;
}

int
main() {
	VoterApp app;
	app.Run();
	return 0;
}