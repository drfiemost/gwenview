/*
Copyright 2004 Jonathan Riddell <jr@jriddell.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef __gvdirpart_h__
#define __gvdirpart_h__

#include <kparts/part.h>
#include <kparts/browserextension.h>

// Forward declarations
class QSplitter;
class KAboutData;
class KAction;
class GVScrollPixmapView;
class GVFileViewStack;
class GVDocument;
class GVSlideShow;

class GVDirPart;

/**
 * The browser extension is an attribute of GVImagePart and provides
 * some services to Konqueror.  All Konqueror KParts have one.
 */
class GVDirPartBrowserExtension: public KParts::BrowserExtension {
	Q_OBJECT

public:
	GVDirPartBrowserExtension(GVDirPart* viewPart, const char* name=0L);
	~GVDirPartBrowserExtension();

public slots:
	void contextMenu();

	void updateActions();
	void refresh();

	void copy();
	void cut();
	void trash();
	void del();
	void editMimeType();

	void print();
private:
	GVDirPart* mGVDirPart;
};

/**
 * A Read Only KPart to browse directories and their images using Gwenview
 */
class GVDirPart : public KParts::ReadOnlyPart {
	Q_OBJECT
public:
	GVDirPart(QWidget*, const char*, QObject*, const char*, const QStringList &);
	virtual ~GVDirPart();

	/**
	 * Return information about the part
	 */
	static KAboutData* createAboutData();

	/**
	 * Returns the name of the current file in the pixmap
	 */
	KURL pixmapURL();

	/**
	 * Print the image being viewed if there is one
	 */
	void print();
protected:
	void partActivateEvent(KParts::PartActivateEvent* event);

	/**
	 * Unused because openURL() is implemented but required to be
	 * implemented.
	 */
	virtual bool openFile();
	
	/**
	 * Tell the widgets the URL to browse.  Sets the window
	 * caption and saves URL to m_url (important for history and
	 * others).
	 */
	virtual bool openURL(const KURL& url);

protected slots:
	/**
	 * Turns the slide show on or off
	 */
	void toggleSlideShow();

	/**
	 * Sets Konqueror's caption, statusbar and emits completed().
	 * Called by loaded() signal in GVDocument
	 */
	void loaded(const KURL& url);

	/**
	 * Rotates the current image 90 degrees clockwise
	 */
	void rotateRight();

	void directoryChanged(const KURL& dirURL);

protected:
	/**
	 * The component's widget, contains the files view on the left
	 * and scroll view on the right.
	 */
	QSplitter* mSplitter;

	/**
	 * Scroll widget
	 */
	GVScrollPixmapView* mPixmapView;

	/**
	 * Holds the image
	 */
	GVDocument* mDocument;

	/**
	 * Shows the directory's files and folders
	 */

	GVFileViewStack* mFilesView;

	/**
	 * This inherits from KParts::BrowserExtention and supplies
	 * some extra functionality to Konqueror.
	 */
	GVDirPartBrowserExtension* mBrowserExtension;

	/**
	 * Action turns on slide show
	 */
	KToggleAction* mToggleSlideShow;
	GVSlideShow* mSlideShow;
};

#endif
