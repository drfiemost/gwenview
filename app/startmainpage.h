// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
#ifndef STARTMAINPAGE_H
#define STARTMAINPAGE_H

// Qt
#include <QFrame>

// KDE

// Local

class QModelIndex;
class QPalette;
class QShowEvent;

class KUrl;

namespace Gwenview
{

class GvCore;
class ThumbnailView;

struct StartMainPagePrivate;
class StartMainPage : public QFrame
{
    Q_OBJECT
public:
    StartMainPage(QWidget* parent, GvCore*);
    ~StartMainPage();

    void applyPalette(const QPalette&);
    ThumbnailView* recentFoldersView() const;

Q_SIGNALS:
    void urlSelected(const KUrl& url);

public Q_SLOTS:
    void loadConfig();

protected:
    virtual void showEvent(QShowEvent*);

private Q_SLOTS:
    void slotListViewActivated(const QModelIndex& index);
    void showRecentFoldersViewContextMenu(const QPoint& pos);
    void slotTagViewClicked(const QModelIndex& index);

private:
    StartMainPagePrivate* const d;
};

} // namespace

#endif /* STARTMAINPAGE_H */
