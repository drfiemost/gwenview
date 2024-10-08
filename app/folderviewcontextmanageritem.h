// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
#ifndef FOLDERVIEWCONTEXTMANAGERITEM_H
#define FOLDERVIEWCONTEXTMANAGERITEM_H

// Qt

// KDE

// Local
#include "abstractcontextmanageritem.h"

class QModelIndex;

class KUrl;

namespace Gwenview
{

struct FolderViewContextManagerItemPrivate;
class FolderViewContextManagerItem : public AbstractContextManagerItem
{
    Q_OBJECT
public:
    FolderViewContextManagerItem(ContextManager*);
    ~FolderViewContextManagerItem();

Q_SIGNALS:
    void urlChanged(const KUrl&);

private Q_SLOTS:
    void slotCurrentDirUrlChanged(const KUrl&);
    void expandToSelectedUrl();
    void slotRowsInserted(const QModelIndex&, int start, int end);
    void slotActivated(const QModelIndex&);

private:
    friend struct FolderViewContextManagerItemPrivate;
    FolderViewContextManagerItemPrivate* const d;
};

} // namespace

#endif /* FOLDERVIEWCONTEXTMANAGERITEM_H */
