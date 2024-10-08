// vim: set tabstop=4 shiftwidth=4 expandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2006 Aurelien Gateau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef MIMETYPEUTILS_H
#define MIMETYPEUTILS_H

#include <lib/gwenviewlib_export.h>

// Local
class QStringList;

class KFileItem;
class KUrl;

namespace Gwenview
{

namespace MimeTypeUtils
{

GWENVIEWLIB_EXPORT const QStringList& rasterImageMimeTypes();
GWENVIEWLIB_EXPORT const QStringList& svgImageMimeTypes();
GWENVIEWLIB_EXPORT const QStringList& imageMimeTypes();

GWENVIEWLIB_EXPORT QString urlMimeType(const KUrl&);

/**
 * Finds mimetype by downloading the beginning of the url
 */
GWENVIEWLIB_EXPORT QString urlMimeTypeByContent(const KUrl&);

enum Kind {
    KIND_UNKNOWN      = 0,
    KIND_DIR          = 1,
    KIND_ARCHIVE      = 1 << 2,
    KIND_FILE         = 1 << 3,
    KIND_RASTER_IMAGE = 1 << 4,
    KIND_SVG_IMAGE    = 1 << 5,
    KIND_VIDEO        = 1 << 6
};
Q_DECLARE_FLAGS(Kinds, Kind)

GWENVIEWLIB_EXPORT Kind fileItemKind(const KFileItem&);
GWENVIEWLIB_EXPORT Kind urlKind(const KUrl&);
GWENVIEWLIB_EXPORT Kind mimeTypeKind(const QString& mimeType);

} // namespace MimeTypeUtils

} // namespace Gwenview

Q_DECLARE_OPERATORS_FOR_FLAGS(Gwenview::MimeTypeUtils::Kinds)

#endif /* MIMETYPEUTILS_H */
