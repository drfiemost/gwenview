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
#ifndef PRELOADER_H
#define PRELOADER_H

// Qt
#include <QObject>

// KDE

// Local

class QSize;

class KUrl;

namespace Gwenview
{

struct PreloaderPrivate;

/**
 * This class preloads a document to fit a specific size.
 */
class Preloader : public QObject
{
    Q_OBJECT
public:
    Preloader(QObject* parent);
    ~Preloader();

    void preload(const KUrl&, const QSize&);

private Q_SLOTS:
    void doPreload();

private:
    PreloaderPrivate* const d;
};

} // namespace

#endif /* PRELOADER_H */
