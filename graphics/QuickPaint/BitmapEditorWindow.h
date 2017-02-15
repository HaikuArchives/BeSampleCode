/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef BitmapEditorWindow_h
#define BitmapEditorWindow_h

class BBitmapDocument;
class BBitmapEditor;
class BMatrix;
class BLayerView;

class BBitmapEditorWindow : public BWindow
{
	protected:
		BMenuBar	*	m_menu;
		BBitmapEditor *	m_editor;
		BScrollView *	m_scrollView;
		BLayerView *	m_layers;
		BView *			m_layerContainer;

	public:

						BBitmapEditorWindow(BRect r, BBitmapDocument *bitmap);

		BBitmapEditor *	Editor() { return m_editor; };

		virtual	bool	QuitRequested();
		virtual void	MessageReceived(BMessage *msg);
				void	RedoSizes();
				void	ResizeToMax();

};

#endif
