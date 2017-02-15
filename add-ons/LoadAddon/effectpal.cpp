//****************************************
//effectpal.cpp
//****************************************
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <image.h>

#include "effectpal.h"
#include "effect.h"
#include <Application.h>
#include <Roster.h>
#include <Path.h>
#include <Directory.h>
#include <stdio.h>

//****************************************
//EffectPal class functions

//constructor
EffectPal::EffectPal( BRect frame, const char *title, window_type type, uint32 flags, 
         uint32 workspaces )
	: BWindow( frame, title, type, flags, workspaces )
{	
}

//destructor
EffectPal::~EffectPal()
{
}

//Init
void EffectPal::Init( void )
{
	image_id	addonId;
   	status_t 	err = B_NO_ERROR; 
   	Effect*		peffect = NULL;
   	BPoint		point(0,0), apoint;
   	Effect*		(*NewEffect)( image_id );	//addon function prototype
	app_info info; 
    BPath path; 
    
   	//look in app directory for effects
    be_app->GetAppInfo(&info); 
    BEntry entry(&info.ref); 
    entry.GetPath(&path); 
    path.GetParent(&path);
	path.Append("Effects");

	BDirectory dir( path.Path() );

   	//load all effects
	while( err == B_NO_ERROR ){
		err = dir.GetNextEntry( (BEntry*)&entry, TRUE );			
		if( entry.InitCheck() != B_NO_ERROR ){
			break;
		}
		if( entry.GetPath(&path) != B_NO_ERROR ){
			printf( "entry.GetPath failed\n" );
		}else{
			addonId = load_add_on( path.Path() );
			if( addonId < 0 ){
				printf( "load_add_on( %s ) failed\n", path.Path() );
			}else{
				printf( "load_add_on( %s ) successful!\n", path.Path() );
				if( get_image_symbol( addonId, 
									"NewEffect", 
									B_SYMBOL_TYPE_TEXT, 
									(void **)&NewEffect) ){
					printf( "get_image_symbol( NewEffect ) failed\n" );
					unload_add_on( addonId );
				}else{
					peffect = (*NewEffect)( addonId );
					if( !peffect ){
						printf( "failed to create new effect\n" );
					}else{
						peffect->Init( this, point );
						peffect->GetButtonSize( (BPoint*) &apoint );
						point.y += apoint.y;
					}
				}
			}
		}
	}
}

bool EffectPal::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}

//****************************************