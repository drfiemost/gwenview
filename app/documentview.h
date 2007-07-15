/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

// Qt
#include <QStackedWidget>

class KStatusBar;
class KUrl;

namespace KParts { class ReadOnlyPart; class MainWindow; }

namespace Gwenview {

class ContextManager;
class DocumentViewPrivate;

/**
 * Holds the active document view, or show a message if there is no active
 * document
 */
class DocumentView : public QStackedWidget {
	Q_OBJECT
public:
	DocumentView(QWidget* parent, KParts::MainWindow* mainWindow);
	~DocumentView();

	// FIXME: REFACTOR
	void createPartForUrl(const KUrl& url);

	/**
	 * Initialize the context manager, this is needed because DocumentView must
	 * notify the context manager whenever its ImageView changes
	 */
	void setContextManager(ContextManager*);

	/**
	 * Set the current view. If view is 0, show a "no document" message
	 */
	void setView(QWidget* view);

	/**
	 * Reset the view
	 */
	void reset();

	QWidget* viewContainer() const;

	KStatusBar* statusBar() const;

	KParts::ReadOnlyPart* part() const;

	virtual QSize sizeHint() const;

Q_SIGNALS:
	/**
	 * Emitted whenever the part changes. Main window should call createGui on
	 * it.
	 */
	void partChanged(KParts::ReadOnlyPart*);

private:
	DocumentViewPrivate* const d;
};

} // namespace

#endif /* DOCUMENTVIEW_H */
