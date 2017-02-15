/* whack.h */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef WHACK_H
#define WHACK_H

#include "addon.h"

#define QUICKPAINT_SUITE 1

#define APP_SIGNATURE					"application/x-vnd.Be-whack"

#if QUICKPAINT_SUITE
#define QUICKPAINT_SIGNATURE			"application/x-vnd.Be-QuickPaint"
#define MENU_QUICKPAINT_SEND_FRAME		'Mqps'
#endif

/* message codes */
#define MENU_OPEN						'Mopn'
#define MENU_SAVE_AS					'Msav'
#define MENU_ABOUT						'Mabt'

#define MENU_ADD_FAVORITE				'Madf'
#define MENU_REMOVE_FAVORITE			'Mrmf'

#define MENU_RESET						'Mres'
#define MENU_FULL_SCREEN				'Mful'
#define MENU_SETTINGS					'Mset'

#define MENU_DEBUG_DUMP_ADDON			'Mdad'
#define MENU_DEBUG_DUMP_FAVORITES		'Mdfv'

#define FILE_OPENED						'Fopn'
#define FILE_SAVED						'Fsav'

#define FAVORITE_SELECTED				'favs'
#define BUTTON_COMPILE					'cmpl'

extern BMessage settings;

class WhackView : public BView
{
public:
	WhackView(BRect r);
	
	void MessageReceived(BMessage *msg);
	void MouseDown(BPoint p);

private:
	bigtime_t			_last_hit;
};

class WhackWindow : public BDirectWindow
{
public:
	WhackWindow(BRect r);
	~WhackWindow();
	
	void MessageReceived(BMessage *msg);
	bool QuitRequested();
	void DirectConnected(direct_buffer_info *info);
	
private:
#if QUICKPAINT_SUITE
	/* QuickPaint cross-application intercompatibility ORB layer */
	void CheckForQuickPaint(void);
	BMenu *_quickpaint_menu;
	bool _quickpaint_present;
#endif

	void ErrorAlert(const char *error_str);
	
	/* favorites */
	void LoadFavorites(void);
	void UnloadFavorites(void);
	void AddFavorite(void);
	void RemoveFavorite(const char *name);
	bool IsFavorite(const char *name);
	
	/* addon stuff */
	void ChangeAddon(const char *path);
	void ChangeAddon(whack_addon *addon);
	
	/* UI setup */
	float InitMenuBar(BRect r);
	float InitExpression(BRect r);
	
	/* thread entry functions */
	static int32 _compile_(void *data);
	int32 Compile(void);
	
	static int32 _draw_loop_(void *data);
	int32 DrawLoop(void);

	BList				*_favorites;
	BDirectory			_favorites_dir;
	BMenuItem			*_favorites_add;
	BMenuItem			*_favorites_remove;
	
	WhackView			*_whack_view;
	BMenuBar			*_menubar;
	BView				*_bottom_view;
	BTextControl		*_expression;
	BButton				*_compile;
	int32				_menubar_height;
	int32				_expression_height;

	BFilePanel			*_open_panel;
	BFilePanel			*_save_panel;
				
	BLocker				_lock;
	
	thread_id			_draw_thread;
	thread_id			_compile_thread;
			
	whack_addon			*_addon;
	int32				_frame_count;
	int32				_pixel_count;
	uint8				*_framebuffer;	
	int32				_bytes_per_row;
	color_space			_mode;
	clipping_rect		_bounds;
	
	clipping_rect		*_rects;
	uint32				_nrects;

	bool				_dirty;
	bool				_connected;
	bool				_disabled;
};

#endif /* WHACK_H */
