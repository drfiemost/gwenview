// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
// Self
#include "thumbnailviewhelper.moc"

#include <config-gwenview.h>

// Qt
#include <QAction>
#include <QCursor>

// KDE
#include <KActionCollection>
#include <KDebug>
#include <KLocale>
#include <KMenu>

// Local
#include <lib/document/documentfactory.h>
#include "fileoperations.h"

namespace Gwenview
{

struct ThumbnailViewHelperPrivate
{
    KActionCollection* mActionCollection;
    KUrl mCurrentDirUrl;

    void addActionToMenu(KMenu& popup, const char* name)
    {
        QAction* action = mActionCollection->action(name);
        if (!action) {
            kWarning() << "Unknown action" << name;
            return;
        }
        if (action->isEnabled()) {
            popup.addAction(action);
        }
    }
};

ThumbnailViewHelper::ThumbnailViewHelper(QObject* parent, KActionCollection* actionCollection)
: AbstractThumbnailViewHelper(parent)
, d(new ThumbnailViewHelperPrivate)
{
    d->mActionCollection = actionCollection;
}

ThumbnailViewHelper::~ThumbnailViewHelper()
{
    delete d;
}

void ThumbnailViewHelper::setCurrentDirUrl(const KUrl& url)
{
    d->mCurrentDirUrl = url;
}

void ThumbnailViewHelper::showContextMenu(QWidget* parent)
{
    KMenu popup(parent);
    if (d->mCurrentDirUrl.protocol() == "trash") {
        d->addActionToMenu(popup, "file_restore");
        d->addActionToMenu(popup, "file_delete");
        popup.addSeparator();
        d->addActionToMenu(popup, "file_show_properties");
    } else {
        d->addActionToMenu(popup, "file_create_folder");
        popup.addSeparator();
        d->addActionToMenu(popup, "file_rename");
        d->addActionToMenu(popup, "file_trash");
        d->addActionToMenu(popup, "file_delete");
        popup.addSeparator();
        d->addActionToMenu(popup, "file_copy_to");
        d->addActionToMenu(popup, "file_move_to");
        d->addActionToMenu(popup, "file_link_to");
        popup.addSeparator();
        d->addActionToMenu(popup, "file_open_with");
        popup.addSeparator();
        d->addActionToMenu(popup, "file_show_properties");
    }
    popup.exec(QCursor::pos());
}

void ThumbnailViewHelper::showMenuForUrlDroppedOnViewport(QWidget* parent, const KUrl::List& lst)
{
    showMenuForUrlDroppedOnDir(parent, lst, d->mCurrentDirUrl);
}

void ThumbnailViewHelper::showMenuForUrlDroppedOnDir(QWidget* parent, const KUrl::List& urlList, const KUrl& destUrl)
{
    FileOperations::showMenuForDroppedUrls(parent, urlList, destUrl);
}

} // namespace
