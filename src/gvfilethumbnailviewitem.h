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

#ifndef GVFILETHUMBNAILVIEWITEM_H
#define GVFILETHUMBNAILVIEWITEM_H

// Qt includes
#include <qiconview.h>
#include <qstring.h>


class QFontMetrics;
class QPixmap;
class KFileItem;
class KWordWrap;


/**
 * We override the QIconViewItem to control the look of selected items
 * and get a pointer to our KFileItem
 */
class GVFileThumbnailViewItem : public QIconViewItem {
public:
	GVFileThumbnailViewItem(QIconView* parent,const QString& text,const QPixmap& icon, KFileItem* fileItem);
	~GVFileThumbnailViewItem();

	KFileItem* fileItem() const { return mFileItem; }

	void setInfoText(const QString&);

protected:
	void paintItem(QPainter* painter, const QColorGroup& colorGroup);
	void calcRect( const QString& text_=QString::null );
	void truncateText(const QFontMetrics&);
	void paintFocus(QPainter*, const QColorGroup&) {}
	bool acceptDrop(const QMimeSource*) const;
	void dropped(QDropEvent*, const QValueList<QIconDragItem>&);
	
	KFileItem* mFileItem;
	KWordWrap* mWordWrap;
	QString mTruncatedText;
	QString mInfoText;
};

#endif
