// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�lien G�teau
 
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
#ifndef GVDOCUMENT_H
#define GVDOCUMENT_H

// Qt 
#include <qcstring.h>
#include <qobject.h>
#include <qimage.h>

// KDE 
#include <kurl.h>
#include <kprinter.h>

// Local 
#include "gvimageutils.h"
#include "gvbusylevelmanager.h"

class GVDocumentPrivate;
class GVDocumentImpl;

/**
 * The application document.
 * It knows what the current url is.
 */
class GVDocument : public QObject {
Q_OBJECT
public:
	enum ModifiedBehavior { ASK=0, SAVE_SILENTLY=1, DISCARD_CHANGES=2 };
	enum CommentState { NONE=0, READ_ONLY=1, VALID=READ_ONLY, WRITABLE=3 };
	
	GVDocument(QObject*);
	~GVDocument();

	// Properties
	const QImage& image() const;
	KURL url() const;
	const KURL& dirURL() const;
	const QString& filename() const;
	const char* imageFormat() const;

	// Convenience methods
	bool isNull() const { return image().isNull(); }
	int width() const { return image().width(); }
	int height() const { return image().height(); }

	void setModifiedBehavior(ModifiedBehavior);
	ModifiedBehavior modifiedBehavior() const;

	GVDocument::CommentState commentState() const;
	QString comment() const;
	void setComment(const QString&);
	
public slots:
	void setURL(const KURL&);
	void setDirURL(const KURL&);
	void setFilename(const QString&);
	void reload();

	/**
	 * Save to the current file.
	 * Returns false if failed.
	 */
	bool save();
	void saveAs();
	
	/** print the selected file */
	void print(KPrinter *pPrinter);
	
	/**
	 * If the image has been modified, prompt the user to save the changes.
	 * Returns true if it's safe to switch to another image.
	 */
	bool saveBeforeClosing();

	// "Image manipulation"
	void modify(GVImageUtils::Orientation);

signals:
	/**
	 * Emitted when the class starts to load the image.
	 */
	void loading();

	/**
	 * Emitted when the class has finished loading the image.
	 * Also emitted if the image could not be loaded.
	 */
	void loaded(const KURL& dirURL,const QString& filename);

	/**
	 * Emitted when the image has been modified.
	 */
	void modified();

	/**
	 * Emitted when the image has been saved on disk.
	 */
	void saved(const KURL& url);

	/**
	 * Emitted when the image has been reloaded.
	 */ 
	void reloaded(const KURL& url);

	/**
	 * Emitted during loading to show a part of the image must be refreshed
	 */
	void rectUpdated(const QRect& rect);

	/**
	 * Emitted during loading, when the size is known
	 */
	void sizeUpdated(int width, int height);

private slots:
	void slotFinished(bool success);
	void slotLoading();
	void slotLoaded();
	void slotBusyLevelChanged(GVBusyLevel level);
	
private:
	friend class GVDocumentImpl;

	GVDocumentPrivate* d;

	// These methods are used by GVDocumentImpl and derived
	void switchToImpl(GVDocumentImpl*);
	void setImage(QImage);
	void setImageFormat(const char*);
	
	void reset();
	void load();
	bool saveInternal(const KURL&,const char* format);
	void doPaint(KPrinter *pPrinter, QPainter *p); 
};


#endif
