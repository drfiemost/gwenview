// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�ien G�eau

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

#include <assert.h>

// Qt 
#include <qbitmap.h>
#include <qcolor.h>
#include <qcursor.h>
#include <qdatetime.h>
#include <qevent.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpopupmenu.h>
#include <qstyle.h>
#include <qtimer.h>
#include <qregexp.h>

// KDE 
#include <kaction.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kpropsdlg.h>
#include <kstandarddirs.h>
#include <kstdaction.h>
#include <kurldrag.h>
#include <kapplication.h>

// Local
#include "fileoperation.h"
#include "gvexternaltoolmanager.h"
#include "gvexternaltoolcontext.h"
#include "gvdocument.h"
#include "gvimageutils/gvimageutils.h"
#include "gvbusylevelmanager.h"
#include "gvscrollpixmapviewtools.h"

#include "gvscrollpixmapview.moc"

/*

Coordinates:

The image can be zoomed, can have a position offset, and additionally there is
QScrollView's viewport. This means there are several coordinate systems.



Let's start from simple things. Viewport ignored, zoom ignored:

 A-----------------------------------
 |                                  |
 |                                  |
 |     B---------------------       |
 |     |                    |       |
 |     |                    |       |
 |     |                    |       |
 |     |                    |       |
 |     |                    |       |
 |     |                    |       |
 |     ---------------------C       |
 |                                  |
 |                                  |
 ------------------------------------


The inner rectangle is the image, outer rectangle is the widget.
A = [ 0, 0 ]
B = [ mXOffset, mYOffset ]
C = B + [ mDocument->width(), mDocument->height() ]



The same, additionally the image is zoomed.

A = [ 0, 0 ]
B = [ mXOffset, mYOffset ]
C = [ mZoom * mDocument->width(), mZoom * mDocument->height()) ]

The groups of functions imageToWidget() and widgetToImage() do conversions
between the image and widget coordinates, i.e. imageToWidget() accepts coordinates
in the image (original,not zoomed,image's topleft corner is [0,0]) and returns
coordinates in the picture above, widgetToImage() works the other way around.

There's no bounds checking, so widgetToImage( A ) in the example above would
return image coordinate with negative x,y.

The widgetToImage() functions round the values (in order to have the conversion
as approximate as possible). However when converting from widget to image and back
this can result in the final rectangle being smaller than the original.
The widgetToImageBounding() function converts from widget to image coordinates
(by adding ceil(1/mZoom) in all directions) in a way which makes sure the reverse
conversion will be at least as large as the original geometry.

There are no conversion functions for only width/height, as their conversion
depends on the position (because of the rounding etc.).



For conversions from/to QScrollView's viewport, usually QScrollView methods should
be used: contentsX(), contentsY(), contentsWidth(), contentsHeight(), visibleWidth(),
visibleHeight(), contentsToViewport() and viewportToContents().

*/

const char* CONFIG_OSD_MODE="osd mode";
const char* CONFIG_FREE_OUTPUT_FORMAT="free output format";
const char* CONFIG_SMOOTH_SCALE="smooth scale";
const char* CONFIG_DELAYED_SMOOTHING="delayed smoothing";
const char* CONFIG_ENLARGE_SMALL_IMAGES="enlarge small images";
const char* CONFIG_SHOW_SCROLL_BARS="show scroll bars";
const char* CONFIG_MOUSE_WHEEL_SCROLL="mouse wheel scrolls image";
const char* CONFIG_LOCK_ZOOM="lock zoom";
const char* CONFIG_AUTO_ZOOM="auto zoom";
const char* CONFIG_AUTO_ZOOM_BROWSE="auto zoom browse";
const char* CONFIG_BACKGROUND_COLOR="background color";
const char* CONFIG_MAX_REPAINT_SIZE = "max repaint size";
const char* CONFIG_MAX_SCALE_REPAINT_SIZE = "max scale repaint size";
const char* CONFIG_MAX_SMOOTH_REPAINT_SIZE = "max smooth repaint size";

const int AUTO_HIDE_TIMEOUT=2000;

const double MAX_ZOOM=16.0; // Same value as GIMP

const int DEFAULT_MAX_REPAINT_SIZE = 10000;
const int LIMIT_MAX_REPAINT_SIZE = 10000000;

const int FULLSCREEN_LABEL_RADIUS = 10;


struct GVScrollPixmapView::Private {
	GVDocument* mDocument;
	QTimer* mAutoHideTimer;
	QLabel* mFullScreenLabel;
	
	QColor mBackgroundColor;
	OSDMode mOSDMode;
	QString mFreeOutputFormat;
	GVImageUtils::SmoothAlgorithm mSmoothAlgorithm;
	bool mDelayedSmoothing;
	bool mEnlargeSmallImages;
	bool mShowScrollBars;
	bool mMouseWheelScroll;
	Tools mTools;

	ToolID mToolID;

	// Offset to center images
	int mXOffset, mYOffset;

	// Zoom info
	double mZoom;

	// Our actions
	KToggleAction* mAutoZoom;
	KAction* mZoomIn;
	KAction* mZoomOut;
	KAction* mResetZoom;
	KToggleAction* mLockZoom;
	KActionCollection* mActionCollection;

	// Object state info
	bool mFullScreen;
	bool mOperaLikePrevious; // Flag to avoid showing the popup menu on Opera like previous
	double mZoomBeforeAuto;
	int mXCenterBeforeAuto, mYCenterBeforeAuto;
	
	QMap< long long, PendingPaint > mPendingPaints;
	QRegion mPendingNormalRegion;
	QRegion mPendingSmoothRegion;
	int mPendingOperations;
	QTimer mPendingPaintTimer;
	bool mSmoothingSuspended;
	bool mEmptyImage;
	int mMaxRepaintSize;
	int mMaxScaleRepaintSize;
	int mMaxSmoothRepaintSize;

	int imageToWidgetX( int x ) const {
		if( mZoom == 1.0 ) return x + mXOffset;
		return int( round( x * mZoom )) + mXOffset;
	}

	int imageToWidgetY( int y ) const {
		if( mZoom == 1.0 ) return y + mYOffset;
		return int( round( y * mZoom )) + mYOffset;
	}

	QPoint imageToWidget( const QPoint& p ) const {
		return QPoint( imageToWidgetX( p.x()), imageToWidgetY( p.y()));
	}

	QRect imageToWidget( const QRect& r ) const {
		return QRect( imageToWidget( r.topLeft()), imageToWidget( r.bottomRight()));
	}

	int widgetToImageX( int x ) const {
		if( mZoom == 1.0 ) return x - mXOffset;
		return int( round( ( x - mXOffset ) / mZoom ));
	}

	int widgetToImageY( int y ) const {
		if( mZoom == 1.0 ) return y - mYOffset;
		return int( round( ( y - mYOffset ) / mZoom ));
	}

	QPoint widgetToImage( const QPoint& p ) const {
		return QPoint( widgetToImageX( p.x()), widgetToImageY( p.y()));
	}

	QRect widgetToImage( const QRect& r ) const {
		return QRect( widgetToImage( r.topLeft()), widgetToImage( r.bottomRight()));
	}

	QRect widgetToImageBounding( const QRect& r ) const {
		int extra = mZoom == 1.0 ? 0 : int( ceil( 1 / mZoom ));
		QRect ret( widgetToImage( r.topLeft()), widgetToImage( r.bottomRight()));
		ret.addCoords( -extra, -extra, extra, extra );
		return ret;
	}

};


class GVScrollPixmapView::EventFilter : public QObject {
public:
	EventFilter(GVScrollPixmapView* parent)
	: QObject(parent) {}
		
	bool eventFilter(QObject*, QEvent* event) {
		switch (event->type()) {
		case QEvent::KeyPress:
		case QEvent::KeyRelease:
		case QEvent::AccelOverride:
			return static_cast< GVScrollPixmapView* >( parent())
						->viewportKeyEvent(static_cast<QKeyEvent*>(event));
		default:
			break;
		}
		return false;
	}
};



GVScrollPixmapView::GVScrollPixmapView(QWidget* parent,GVDocument* document, KActionCollection* actionCollection)
: QScrollView(parent,0L,WResizeNoErase|WRepaintNoErase|WPaintClever)
{
	d=new Private;
	d->mDocument=document;
	d->mAutoHideTimer=new QTimer(this);
	d->mFullScreenLabel=new QLabel(this);
	d->mToolID=SCROLL;
	d->mXOffset=0;
	d->mYOffset=0;
	d->mZoom=1;
	d->mActionCollection=actionCollection;
	d->mFullScreen=false;
	d->mOperaLikePrevious=false;
	d->mZoomBeforeAuto=1;
	d->mPendingOperations= 0 ;
	d->mSmoothingSuspended= false ;
	d->mEmptyImage= false ;
	d->mMaxRepaintSize= DEFAULT_MAX_REPAINT_SIZE ;
	d->mMaxScaleRepaintSize= DEFAULT_MAX_REPAINT_SIZE ;
	d->mMaxSmoothRepaintSize= DEFAULT_MAX_REPAINT_SIZE ;

	setFocusPolicy(StrongFocus);
	setFrameStyle(NoFrame);
	setAcceptDrops( true );
	viewport()->setAcceptDrops( true );

	d->mTools[SCROLL]=new ScrollTool(this);
	d->mTools[ZOOM]=new ZoomTool(this);
	d->mTools[d->mToolID]->updateCursor();

	// Init full screen label
	d->mFullScreenLabel->setBackgroundColor(white);
	QFont font=d->mFullScreenLabel->font();
	font.setWeight(QFont::Bold);
	d->mFullScreenLabel->setFont(font);
    d->mFullScreenLabel->move(2, 2);
	d->mFullScreenLabel->hide();
	
	// Create actions
	d->mAutoZoom=new KToggleAction(i18n("&Auto Zoom"),"viewmagfit",0,d->mActionCollection,"view_zoom_auto");
	connect(d->mAutoZoom,SIGNAL(toggled(bool)),
		this,SLOT(setAutoZoom(bool)) );

	d->mZoomIn=KStdAction::zoomIn(this,SLOT(slotZoomIn()),d->mActionCollection);

	d->mZoomOut=KStdAction::zoomOut(this,SLOT(slotZoomOut()),d->mActionCollection);

	d->mResetZoom=KStdAction::actualSize(this,SLOT(slotResetZoom()),d->mActionCollection);
	d->mResetZoom->setIcon("viewmag1");

	d->mLockZoom=new KToggleAction(i18n("&Lock Zoom"),"lock",0,d->mActionCollection,"view_zoom_lock");

	// Connect to some interesting signals
	connect(d->mDocument,SIGNAL(loaded(const KURL&)),
		this,SLOT(slotLoaded()) );

	connect(d->mDocument,SIGNAL(loading()),
		this,SLOT( loadingStarted()) );

	connect(d->mDocument,SIGNAL(modified()),
		this,SLOT(slotModified()) );

	connect(d->mDocument, SIGNAL(sizeUpdated(int, int)),
		this, SLOT(slotImageSizeUpdated()) );

	connect(d->mDocument, SIGNAL(rectUpdated(const QRect&)),
		this, SLOT(slotImageRectUpdated(const QRect&)) );

	connect(d->mAutoHideTimer,SIGNAL(timeout()),
		this,SLOT(hideCursor()) );

	connect(&d->mPendingPaintTimer,SIGNAL(timeout()),
		this,SLOT(checkPendingOperations()) );

	connect(GVBusyLevelManager::instance(),SIGNAL(busyLevelChanged(GVBusyLevel)),
		this,SLOT(slotBusyLevelChanged(GVBusyLevel) ));

	// This event filter is here to make sure the pixmap view is aware of the changes
	// in the keyboard modifiers, even if it isn't focused. However, making this widget
	// itself the filter would lead to doubled paint events, because QScrollView
	// installs an event filter on its viewport, and doesn't filter out the paint
	// events -> it'd get it twice, first from app filter, second from viewport filter.
	EventFilter* filter=new EventFilter(this);
	kapp->installEventFilter(filter);
}


GVScrollPixmapView::~GVScrollPixmapView() {
	delete d->mTools[SCROLL];
	delete d->mTools[ZOOM];
	delete d;
}


void GVScrollPixmapView::slotLoaded() {
	updateZoomActions();

	if (d->mDocument->isNull()) {
		resizeContents(0,0);
		viewport()->repaint(false);
		return;
	}

	updateContentSize();
	updateImageOffset();
	if (d->mFullScreen && d->mOSDMode!=NONE) updateFullScreenLabel();
	if (doDelayedSmoothing()) scheduleOperation( SMOOTH_PASS );
}


void GVScrollPixmapView::slotModified() {
	if (d->mAutoZoom->isChecked()) {
		setZoom(computeAutoZoom());
	} else {
		updateContentSize();
		updateImageOffset();
		updateZoomActions();
		fullRepaint();
	}
}


void GVScrollPixmapView::loadingStarted() {
	cancelPending();
	d->mSmoothingSuspended = true;
	d->mEmptyImage = true;
	// every loading() signal from GVDocument must be followed by a signal that turns this off
	QPainter painter( viewport());
	painter.eraseRect( viewport()->rect());
}

//------------------------------------------------------------------------
//
// Properties
//
//------------------------------------------------------------------------
KToggleAction* GVScrollPixmapView::autoZoom() const {
	return d->mAutoZoom;
}


KAction* GVScrollPixmapView::zoomIn() const {
	return d->mZoomIn;
}


KAction* GVScrollPixmapView::zoomOut() const {
	return d->mZoomOut;
}


KAction* GVScrollPixmapView::resetZoom() const {
	return d->mResetZoom;
}


KToggleAction* GVScrollPixmapView::lockZoom() const {
	return d->mLockZoom;
}


double GVScrollPixmapView::zoom() const {
	return d->mZoom;
}


bool GVScrollPixmapView::fullScreen() const {
	return d->mFullScreen;
}


QColor GVScrollPixmapView::normalBackgroundColor() const {
	return d->mBackgroundColor;
}


GVScrollPixmapView::OSDMode GVScrollPixmapView::osdMode() const {
	return d->mOSDMode;
}


QString GVScrollPixmapView::freeOutputFormat() const {
	return d->mFreeOutputFormat;
}


void GVScrollPixmapView::setFreeOutputFormat(const QString& format) {
	d->mFreeOutputFormat=format;
}


GVImageUtils::SmoothAlgorithm GVScrollPixmapView::smoothAlgorithm() const {
	return d->mSmoothAlgorithm;
}


bool GVScrollPixmapView::delayedSmoothing() const {
	return d->mDelayedSmoothing;
}


bool GVScrollPixmapView::doDelayedSmoothing() const {
	return d->mDelayedSmoothing && d->mSmoothAlgorithm;
}


bool GVScrollPixmapView::enlargeSmallImages() const {
	return d->mEnlargeSmallImages;
}


bool GVScrollPixmapView::showScrollBars() const {
	return d->mShowScrollBars;
}


bool GVScrollPixmapView::mouseWheelScroll() const {
	return d->mMouseWheelScroll;
}


QPoint GVScrollPixmapView::offset() const {
	return QPoint(d->mXOffset, d->mYOffset);
}


void GVScrollPixmapView::setSmoothAlgorithm(GVImageUtils::SmoothAlgorithm value) {
	if( d->mSmoothAlgorithm == value ) return;
	d->mSmoothAlgorithm = value;
	d->mMaxRepaintSize = DEFAULT_MAX_REPAINT_SIZE; // reset, so that next repaint doesn't
	d->mMaxScaleRepaintSize = DEFAULT_MAX_REPAINT_SIZE; // possibly take longer
	d->mMaxSmoothRepaintSize = DEFAULT_MAX_REPAINT_SIZE; // because of smoothing
	if( doDelayedSmoothing() ) {
		scheduleOperation( SMOOTH_PASS );
	} else {
		fullRepaint();
	}
}


void GVScrollPixmapView::setDelayedSmoothing(bool value) {
	if (d->mDelayedSmoothing==value) return;
	d->mDelayedSmoothing=value;
	d->mMaxRepaintSize = DEFAULT_MAX_REPAINT_SIZE; // reset, so that next repaint doesn't
	d->mMaxScaleRepaintSize = DEFAULT_MAX_REPAINT_SIZE; // possibly take longer
	d->mMaxSmoothRepaintSize = DEFAULT_MAX_REPAINT_SIZE; // because of smoothing
	if( doDelayedSmoothing() ) {
		scheduleOperation( SMOOTH_PASS );
	} else {
		fullRepaint();
	}
}


void GVScrollPixmapView::setEnlargeSmallImages(bool value) {
	d->mEnlargeSmallImages=value;
	if (d->mAutoZoom->isChecked()) {
		setZoom(computeAutoZoom());
	}
}


void GVScrollPixmapView::setOSDMode(GVScrollPixmapView::OSDMode value) {
	d->mOSDMode=value;
}


void GVScrollPixmapView::setShowScrollBars(bool value) {
	d->mShowScrollBars=value;
	updateScrollBarMode();
}


void GVScrollPixmapView::setMouseWheelScroll(bool value) {
	d->mMouseWheelScroll=value;
}


void GVScrollPixmapView::setZoom(double zoom, int centerX, int centerY) {
	int viewWidth=visibleWidth();
	int viewHeight=visibleHeight();
	double oldZoom=d->mZoom;
	d->mZoom=zoom;

	viewport()->setUpdatesEnabled(false);

	updateContentSize();

	// Find the coordinate of the center of the image
	// and center the view on it
	if (centerX==-1) {
		centerX=int( ((viewWidth/2+contentsX()-d->mXOffset)/oldZoom)*d->mZoom );
	}
	if (centerY==-1) {
		centerY=int( ((viewHeight/2+contentsY()-d->mYOffset)/oldZoom)*d->mZoom );
	}
	center(centerX,centerY);

	updateImageOffset();
	updateZoomActions();

	viewport()->setUpdatesEnabled(true);
	fullRepaint();

	emit zoomChanged(d->mZoom);
}


void GVScrollPixmapView::setFullScreen(bool fullScreen) {
	d->mFullScreen=fullScreen;
	viewport()->setMouseTracking(d->mFullScreen);
	if (d->mFullScreen) {
		viewport()->setBackgroundColor(black);
		restartAutoHideTimer();
	} else {
		viewport()->setBackgroundColor(d->mBackgroundColor);
		d->mAutoHideTimer->stop();
		d->mTools[d->mToolID]->updateCursor();
	}
	if (d->mFullScreen && d->mOSDMode!=NONE) {
		updateFullScreenLabel();
		d->mFullScreenLabel->show();
	} else {
		d->mFullScreenLabel->hide();
	}
}


void GVScrollPixmapView::setNormalBackgroundColor(const QColor& color) {
	d->mBackgroundColor=color;
	viewport()->setBackgroundColor(d->mBackgroundColor);
}


//------------------------------------------------------------------------
//
// Overloaded methods
//
//------------------------------------------------------------------------
void GVScrollPixmapView::resizeEvent(QResizeEvent* event) {
	QScrollView::resizeEvent(event);
	if (d->mAutoZoom->isChecked()) {
		setZoom(computeAutoZoom());
	} else {
		updateContentSize();
		updateImageOffset();
	}
}


inline void composite(uint* rgba,uint value) {
	uint alpha=(*rgba) >> 24;
	if (alpha<255) {
		uint alphaValue=(255-alpha)*value;

		uint c1=( ( (*rgba & 0xFF0000) >> 16 ) * alpha + alphaValue ) >> 8;
		uint c2=( ( (*rgba & 0x00FF00) >>  8 ) * alpha + alphaValue ) >> 8;
		uint c3=( ( (*rgba & 0x0000FF) >>  0 ) * alpha + alphaValue ) >> 8;
		*rgba=0xFF000000 + (c1<<16) + (c2<<8) + c3;
	}
}

void GVScrollPixmapView::drawContents(QPainter* painter,int clipx,int clipy,int clipw,int cliph) {
	if( !d->mEmptyImage ) {
		addPendingPaint( false, QRect( clipx, clipy, clipw, cliph ));
	} else {
		// image is empty, simply clear
		painter->eraseRect( clipx, clipy, clipw, cliph );
	}
}

// How this pending stuff works:
// There's a queue of areas to paint (each with bool saying whether it's smooth pass).
// Also, there's a bitfield of pending operations, operations are handled only after
// there's nothing more to paint (so that smooth pass is started).
void GVScrollPixmapView::addPendingPaint( bool smooth, QRect rect ) {
	if( d->mSmoothingSuspended && smooth ) return;

	// try to avoid scheduling already scheduled areas
	QRegion& region = smooth ? d->mPendingSmoothRegion : d->mPendingNormalRegion;
	if( region.intersect( rect ) == QRegion( rect ))
		return; // whole rect has already pending paints
	// at least try to remove the part that's already scheduled
	rect = ( QRegion( rect ) - region ).boundingRect();
	region += rect;
	if( rect.isEmpty())
		return;
	addPendingPaintInternal( smooth, rect );
}

void GVScrollPixmapView::addPendingPaintInternal( bool smooth, QRect rect ) {
	const long long MAX_DIM = 1000000; // if monitors get larger than this, we're in trouble :)
	// QMap will ensure ordering (non-smooth first, top-to-bottom, left-to-right)
	long long key = ( smooth ? MAX_DIM * MAX_DIM : 0 ) + rect.y() * MAX_DIM + rect.x();
	// handle the case of two different paints at the same position (just in case)
	key *= 100;
	bool insert = true;
	while( d->mPendingPaints.contains( key )) {
		if( d->mPendingPaints[ key ].rect.contains( rect )) {
			insert = false;
			break;
		}
		if( rect.contains( d->mPendingPaints[ key ].rect )) {
			break;
		}
		++key;
	}
	if( insert ) {
		d->mPendingPaints[ key ] = PendingPaint( smooth, rect );
	}
	scheduleOperation( CHECK_OPERATIONS );
}

void GVScrollPixmapView::checkPendingOperations() {
	checkPendingOperationsInternal();
	if( d->mPendingPaints.isEmpty() && d->mPendingOperations == 0 ) {
		d->mPendingPaintTimer.stop();
	}
	updateBusyLevels();
}

void GVScrollPixmapView::limitPaintSize( PendingPaint& paint ) {
	// The only thing that makes time spent in performPaint() vary
	// is whether there will be scaling and whether there will be smoothing.
	// So there are three max sizes for each mode.
	int maxSize = d->mMaxRepaintSize;
	if( d->mZoom != 1.0 ) {
		if( paint.smooth || !doDelayedSmoothing() ) {
			maxSize = d->mMaxSmoothRepaintSize;
		} else {
			maxSize = d->mMaxScaleRepaintSize;
		}
	}
	// don't paint more than max_size pixels at a time
	int maxHeight = ( maxSize + paint.rect.width() - 1 ) / paint.rect.width(); // round up
	maxHeight = QMAX( maxHeight, 5 ); // at least 5 lines together
	// can't repaint whole paint at once, adjust height and schedule the rest
	if( maxHeight < paint.rect.height()) {
		QRect remaining = paint.rect;
		remaining.setTop( remaining.top() + maxHeight );
		addPendingPaintInternal( paint.smooth, remaining );
		paint.rect.setHeight( maxHeight );
	}
}


void GVScrollPixmapView::checkPendingOperationsInternal() {
	if( !d->mPendingPaintTimer.isActive()) // suspended
		return;
	while( !d->mPendingPaints.isEmpty()) {
		PendingPaint paint = *d->mPendingPaints.begin();
		d->mPendingPaints.remove( d->mPendingPaints.begin());
		limitPaintSize( paint ); // modifies paint.rect if necessary
		QRegion& region = paint.smooth ? d->mPendingSmoothRegion : d->mPendingNormalRegion;
		region -= paint.rect;
		QRect visibleRect( contentsX(), contentsY(), visibleWidth(), visibleHeight());
		QRect paintRect = paint.rect.intersect( visibleRect );
		if( !paintRect.isEmpty()) {
			QPainter painter( viewport());
			painter.translate( -contentsX(), -contentsY());
			performPaint( &painter, paintRect.x(), paintRect.y(),
				paintRect.width(), paintRect.height(), paint.smooth );
			return;
		}
	}
	if( d->mPendingOperations & SMOOTH_PASS ) {
		d->mSmoothingSuspended = false;
		if( doDelayedSmoothing() ) {
			QRect visibleRect( contentsX(), contentsY(), visibleWidth(), visibleHeight());
			addPendingPaint( true, visibleRect );
		}
		d->mPendingOperations &= ~SMOOTH_PASS;
		return;
	}
}

void GVScrollPixmapView::scheduleOperation( Operation operation )
{
	d->mPendingOperations |= operation;
	slotBusyLevelChanged( GVBusyLevelManager::instance()->busyLevel());
	updateBusyLevels();
}

void GVScrollPixmapView::updateBusyLevels() {
	if( !d->mPendingPaintTimer.isActive()) {
		GVBusyLevelManager::instance()->setBusyLevel( this, BUSY_NONE );
	} else if( !d->mPendingPaints.isEmpty() && !(*d->mPendingPaints.begin()).smooth ) {
		GVBusyLevelManager::instance()->setBusyLevel( this, BUSY_PAINTING );
	} else if(( d->mPendingOperations & SMOOTH_PASS )
		|| ( !d->mPendingPaints.isEmpty() && (*d->mPendingPaints.begin()).smooth )) {
		GVBusyLevelManager::instance()->setBusyLevel( this, BUSY_SMOOTHING );
	} else {
		assert( false );
	}
}

void GVScrollPixmapView::slotBusyLevelChanged( GVBusyLevel level ) {
	bool resume = false;
	if( level <= BUSY_PAINTING
		&& !d->mPendingPaints.isEmpty() && !(*d->mPendingPaints.begin()).smooth ) {
		resume = true;
	} else if( level <= BUSY_SMOOTHING
			&& (( d->mPendingOperations & SMOOTH_PASS )
			|| ( !d->mPendingPaints.isEmpty() && (*d->mPendingPaints.begin()).smooth ))) {
		resume = true;
	}
	if( resume ) {
		d->mPendingPaintTimer.start( 0 );
	} else {
		d->mPendingPaintTimer.stop();
	}
}

// How to do painting:
// When something needs to be erased: QPainter on viewport and eraseRect()
// When whole picture needs to be repainted: fullRepaint()
// When a part of the picture needs to be updated: viewport()->repaint(area,false)
// All other paints will be changed to progressive painting.
void GVScrollPixmapView::fullRepaint() {
	if( !viewport()->isUpdatesEnabled()) return;
	cancelPending();
	viewport()->repaint(false);
}

void GVScrollPixmapView::cancelPending() {
	d->mPendingPaints.clear();
	d->mPendingNormalRegion = QRegion();
	d->mPendingSmoothRegion = QRegion();
	d->mPendingPaintTimer.stop();
	d->mPendingOperations = 0;
	updateBusyLevels();
}

//#define DEBUG_RECTS

// do the actual painting
void GVScrollPixmapView::performPaint( QPainter* painter, int clipx, int clipy, int clipw, int cliph, bool smooth ) {
	#ifdef DEBUG_RECTS
	static QColor colors[4]={QColor(255,0,0),QColor(0,255,0),QColor(0,0,255),QColor(255,255,0) };
	static int numColor=0;
	#endif

	QTime t;
	t.start();

	if (d->mDocument->isNull()) {
		painter->eraseRect(clipx,clipy,clipw,cliph);
		return;
	}

	QRect imageRect = d->widgetToImageBounding( QRect(clipx,clipy,clipw,cliph));
	imageRect = imageRect.intersect( QRect( 0, 0, d->mDocument->width(), d->mDocument->height()));
	if (imageRect.isEmpty()) {
		painter->eraseRect(clipx,clipy,clipw,cliph);
		return;
	}
	QImage image = d->mDocument->image().copy( imageRect );
	QRect widgetRect = d->imageToWidget( imageRect );

	if (image.isNull() || widgetRect.isEmpty()) {
		painter->eraseRect(clipx,clipy,clipw,cliph);
		return;
	}

	int* maxRepaintSize = &d->mMaxRepaintSize;
	if (smooth) {
		if( zoom() != 1.0 ) {
			image=image.convertDepth(32);
			image=GVImageUtils::scale(image,widgetRect.width(),widgetRect.height(), d->mSmoothAlgorithm );
			maxRepaintSize = &d->mMaxSmoothRepaintSize;
		}
	} else {
		if( zoom() != 1.0 ) {
			GVImageUtils::SmoothAlgorithm algo=doDelayedSmoothing()
				?GVImageUtils::SMOOTH_NONE
				:d->mSmoothAlgorithm;
			image=GVImageUtils::scale(image,widgetRect.width(),widgetRect.height(), algo );
			maxRepaintSize = doDelayedSmoothing() ? &d->mMaxScaleRepaintSize : &d->mMaxSmoothRepaintSize;
			
			if( doDelayedSmoothing() && zoom() != 1.0 ) {
				addPendingPaint( true, QRect( clipx, clipy, clipw, cliph ));
			}
		}
	}

	if (image.hasAlphaBuffer()) {
		if (image.depth()!=32) {
			image=image.convertDepth(32);
		}

		bool light;

		int imageXOffset=widgetRect.x()-d->mXOffset;
		int imageYOffset=widgetRect.y()-d->mYOffset;
		int imageWidth=image.width();
		int imageHeight=image.height();
		for (int y=0;y<imageHeight;++y) {
			uint* rgba=(uint*)(image.scanLine(y));
			for(int x=0;x<imageWidth;x++) {
				light= ((x+imageXOffset) & 16) ^ ((y+imageYOffset) & 16);
				composite(rgba,light?192:128);
				rgba++;
			}
		}
		image.setAlphaBuffer(false);
	}

	QRect paintRect( clipx, clipy, clipw, cliph );
	QPixmap buffer( paintRect.size());
	{
		QPainter bufferPainter(&buffer);
		bufferPainter.setBackgroundColor(painter->backgroundColor());
		bufferPainter.eraseRect(0,0,paintRect.width(),paintRect.height());
		bufferPainter.drawImage(widgetRect.topLeft()-paintRect.topLeft(),image);
	}
	painter->drawPixmap(paintRect.topLeft(),buffer);

	if( paintRect.width() * paintRect.height() >= 10000 ) { // ignore small repaints
		// try to do one step in 0.1sec
		int size = paintRect.width() * paintRect.height() * 100 / QMAX( t.elapsed(), 1 );
		*maxRepaintSize = QMIN( LIMIT_MAX_REPAINT_SIZE, QMAX( 10000,
				( size + *maxRepaintSize ) / 2 ));
	}

	#ifdef DEBUG_RECTS
	painter->setPen(colors[numColor]);
	numColor=(numColor+1)%4;
	painter->drawRect(paintRect);
	#endif

	QApplication::flushX();
}


void GVScrollPixmapView::viewportMousePressEvent(QMouseEvent* event) {
	viewport()->setFocus();
	switch (event->button()) {
	case Qt::LeftButton:
		d->mTools[d->mToolID]->leftButtonPressEvent(event);
		break;
	case Qt::RightButton:
		d->mTools[d->mToolID]->rightButtonPressEvent(event);
		break;
	default: // Avoid compiler complain
		break;
	}
}


void GVScrollPixmapView::viewportMouseMoveEvent(QMouseEvent* event) {
	selectTool(event->state(), true);
	d->mTools[d->mToolID]->mouseMoveEvent(event);
	if (d->mFullScreen) {
		restartAutoHideTimer();
	}
}


void GVScrollPixmapView::viewportMouseReleaseEvent(QMouseEvent* event) {
	switch (event->button()) {
	case Qt::LeftButton:
		if (event->stateAfter() & Qt::RightButton) {
			d->mOperaLikePrevious=true;
			emit selectPrevious();
			return;
		}
		d->mTools[d->mToolID]->leftButtonReleaseEvent(event);
		break;

	case Qt::MidButton:
		d->mTools[d->mToolID]->midButtonReleaseEvent(event);
		break;

	case Qt::RightButton:
		if (event->stateAfter() & Qt::LeftButton) {
			emit selectNext();
			return;
		}

		if (d->mOperaLikePrevious) { // Avoid showing the popup menu after Opera like previous
			d->mOperaLikePrevious=false;
		} else {
			d->mTools[d->mToolID]->rightButtonReleaseEvent(event);
		}
		break;

	default: // Avoid compiler complain
		break;
	}
}


bool GVScrollPixmapView::eventFilter(QObject* obj, QEvent* event) {
	switch (event->type()) {
	case QEvent::KeyPress:
	case QEvent::KeyRelease:
	case QEvent::AccelOverride:
		return viewportKeyEvent(static_cast<QKeyEvent*>(event));

	case QEvent::MouseButtonDblClick:
		d->mActionCollection->action("fullscreen")->activate();
		return true;

	// Getting/loosing focus causes repaints, but repainting here is expensive,
	// and there's no need to repaint on focus changes, as the focus is not
	// indicated.
	case QEvent::FocusIn:
	case QEvent::FocusOut:
		return true;

	default:
		break;
	}
	return QScrollView::eventFilter(obj,event);
}


bool GVScrollPixmapView::viewportKeyEvent(QKeyEvent* event) {
	selectTool(event->stateAfter(), false);
	if (d->mFullScreen) {
		restartAutoHideTimer();
	}
	return false;
}


void GVScrollPixmapView::contentsDragEnterEvent(QDragEnterEvent* event) {
	event->accept( QUriDrag::canDecode( event ));
}

void GVScrollPixmapView::contentsDropEvent(QDropEvent* event) {
	KURL::List list;
	if( KURLDrag::decode( event, list )) {
		d->mDocument->setURL( list.first());
	}
}

/**
 * If force is set, the cursor will be updated even if the tool is not
 * different from the current one.
 */
void GVScrollPixmapView::selectTool(ButtonState state, bool force) {
	ToolID oldToolID=d->mToolID;
	if (state & ShiftButton) {
		d->mToolID=ZOOM;
	} else {
		d->mToolID=SCROLL;
	}

	if (d->mToolID!=oldToolID || force) {
		d->mTools[d->mToolID]->updateCursor();
	}
}


void GVScrollPixmapView::wheelEvent(QWheelEvent* event) {
	d->mTools[d->mToolID]->wheelEvent(event);
}


//------------------------------------------------------------------------
//
// Slots
//
//------------------------------------------------------------------------
void GVScrollPixmapView::slotZoomIn() {
	if (d->mAutoZoom->isChecked()) {
		d->mAutoZoom->setChecked(false);
		updateScrollBarMode();
	}
	setZoom(computeZoom(true));
}


void GVScrollPixmapView::slotZoomOut() {
	if (d->mAutoZoom->isChecked()) {
		d->mAutoZoom->setChecked(false);
		updateScrollBarMode();
	}
	setZoom(computeZoom(false));
}


void GVScrollPixmapView::slotResetZoom() {
	if (d->mAutoZoom->isChecked()) {
		d->mAutoZoom->setChecked(false);
		updateScrollBarMode();
	}
	setZoom(1.0);
}


void GVScrollPixmapView::setAutoZoom(bool value) {
	updateScrollBarMode();
	if (value) {
		d->mZoomBeforeAuto=d->mZoom;
		d->mXCenterBeforeAuto=width()/2  + contentsX() + d->mXOffset;
		d->mYCenterBeforeAuto=height()/2 + contentsY() + d->mYOffset;
		setZoom(computeAutoZoom());
	} else {
		setZoom(d->mZoomBeforeAuto, d->mXCenterBeforeAuto, d->mYCenterBeforeAuto);
	}
}


//------------------------------------------------------------------------
//
// Private
//
//------------------------------------------------------------------------
void GVScrollPixmapView::slotImageSizeUpdated() {
	d->mXOffset=0;
	d->mYOffset=0;

	if (d->mAutoZoom->isChecked()) {
		d->mXCenterBeforeAuto=0;
		d->mYCenterBeforeAuto=0;
		setZoom(computeAutoZoom());
	} else {
		horizontalScrollBar()->setValue(0);
		verticalScrollBar()->setValue(0);
	}
	updateImageOffset();
	QRect imageRect = d->imageToWidget( QRect( 0, 0, d->mDocument->width(), d->mDocument->height()));

	QPainter painter( viewport());
	// Top rect
	painter.eraseRect( 0, 0,
		viewport()->width(), imageRect.top());

	// Bottom rect
		painter.eraseRect( 0, imageRect.bottom(),
		viewport()->width(), viewport()->height()-imageRect.bottom());

	// Left rect
	painter.eraseRect( 0, imageRect.top(),
		imageRect.left(), imageRect.height());

	// Right rect
	painter.eraseRect( imageRect.right(), imageRect.top(),
		viewport()->width()-imageRect.right(), imageRect.height());

	// Image area
	painter.setPen(painter.backgroundColor().light(200));
	painter.drawRect(imageRect);
}

void GVScrollPixmapView::slotImageRectUpdated(const QRect& imageRect) {
	d->mEmptyImage = false;
	viewport()->repaint( d->imageToWidget( imageRect ), false );
}


void GVScrollPixmapView::restartAutoHideTimer() {
	d->mAutoHideTimer->start(AUTO_HIDE_TIMEOUT,true);
}


void GVScrollPixmapView::openContextMenu(const QPoint& pos) {
	QPopupMenu menu(this);
	bool noImage=d->mDocument->filename().isEmpty();
	bool validImage=!d->mDocument->isNull();

	// The fullscreen item is always there, to be able to leave fullscreen mode
	// if necessary
	d->mActionCollection->action("fullscreen")->plug(&menu);

	if (validImage) {
		menu.insertSeparator();

		d->mAutoZoom->plug(&menu);
		d->mZoomIn->plug(&menu);
		d->mZoomOut->plug(&menu);
		d->mResetZoom->plug(&menu);
		d->mLockZoom->plug(&menu);
	}

	menu.insertSeparator();

	d->mActionCollection->action("first")->plug(&menu);
	d->mActionCollection->action("previous")->plug(&menu);
	d->mActionCollection->action("next")->plug(&menu);
	d->mActionCollection->action("last")->plug(&menu);

	if (validImage) {
		menu.insertSeparator();

		QPopupMenu* editMenu=new QPopupMenu(&menu);
		d->mActionCollection->action("rotate_left")->plug(editMenu);
		d->mActionCollection->action("rotate_right")->plug(editMenu);
		d->mActionCollection->action("mirror")->plug(editMenu);
		d->mActionCollection->action("flip")->plug(editMenu);
		menu.insertItem( i18n("Edit"), editMenu );

		GVExternalToolContext* externalToolContext=
			GVExternalToolManager::instance()->createContext(
			this, d->mDocument->url());

		menu.insertItem(
			i18n("External Tools"), externalToolContext->popupMenu());
	}

	if (!noImage) {
		menu.insertSeparator();

		menu.connectItem(
			menu.insertItem( i18n("&Rename...") ),
			this,SLOT(renameFile()) );
		menu.connectItem(
			menu.insertItem( i18n("&Copy To...") ),
			this,SLOT(copyFile()) );
		menu.connectItem(
			menu.insertItem( i18n("&Move To...") ),
			this,SLOT(moveFile()) );
		menu.connectItem(
			menu.insertItem( i18n("&Delete") ),
			this,SLOT(deleteFile()) );

		menu.insertSeparator();

		menu.connectItem(
			menu.insertItem( i18n("Properties") ),
			this,SLOT(showFileProperties()) );
	}

	menu.exec(pos);
}


void GVScrollPixmapView::updateScrollBarMode() {
	if (d->mAutoZoom->isChecked() || !d->mShowScrollBars) {
		setVScrollBarMode(AlwaysOff);
		setHScrollBarMode(AlwaysOff);
	} else {
		setVScrollBarMode(Auto);
		setHScrollBarMode(Auto);
	}
}


void GVScrollPixmapView::updateContentSize() {
	resizeContents(
		int(d->mDocument->width()*d->mZoom),
		int(d->mDocument->height()*d->mZoom)	);
}

// QSize.scale() does not exist in Qt 3.0.x
#if QT_VERSION<0x030100
static void sizeScaleMin(QSize* size, int w, int h) {
	int w0 = size->width();
	int h0 = size->height();
	int rw = h * w0 / h0;

	if ( rw <= w ) {
		size->setWidth( rw );
		size->setHeight( h );
	} else {
		size->setWidth( w );
		size->setHeight( w * h0 / w0 );
	}
}
#endif

double GVScrollPixmapView::computeAutoZoom() const {
	if (d->mDocument->isNull()) {
		return 1.0;
	}
	QSize size=d->mDocument->image().size();

#if QT_VERSION>=0x030100
	size.scale(width(),height(),QSize::ScaleMin);
#else
	sizeScaleMin(&size,width(),height());
#endif

	double zoom=double(size.width())/d->mDocument->width();
	if (zoom>1.0 && !d->mEnlargeSmallImages) return 1.0;
	return zoom;
}


double GVScrollPixmapView::computeZoom(bool in) const {
	if (in) {
		if (d->mZoom>=1.0) {
			return floor(d->mZoom)+1.0;
		} else {
			return 1/( ceil(1/d->mZoom)-1.0 );
		}
	} else {
		if (d->mZoom>1.0) {
			return ceil(d->mZoom)-1.0;
		} else {
			return 1/( floor(1/d->mZoom)+1.0 );
		}
	}
}


void GVScrollPixmapView::updateImageOffset() {
	int viewWidth=width();
	int viewHeight=height();

	// Compute d->mXOffset and d->mYOffset in case the image does not fit
	// the view width or height
	int zpixWidth=int(d->mDocument->width() * d->mZoom);
	int zpixHeight=int(d->mDocument->height() * d->mZoom);

	if (zpixWidth>viewWidth && hScrollBarMode()!=AlwaysOff) {
		viewHeight-=horizontalScrollBar()->height();
	}
	if (zpixHeight>viewHeight && vScrollBarMode()!=AlwaysOff) {
		viewWidth-=verticalScrollBar()->width();
	}

	d->mXOffset=QMAX(0,(viewWidth-zpixWidth)/2);
	d->mYOffset=QMAX(0,(viewHeight-zpixHeight)/2);
}


void GVScrollPixmapView::hideCursor() {
	viewport()->setCursor(blankCursor);
}



static void fillRoundRect(QPainter& painter, const QRect& rect, int radius) {
	painter.fillRect(
		rect.left() + radius,
		rect.top(),
		rect.width() - radius*2,
		rect.bottom(),
		painter.brush());

	painter.fillRect(
		rect.left(),
		rect.top() + radius,
		rect.right(),
		rect.height() - radius*2,
		painter.brush());

	painter.drawPie(
		rect.left(),
		rect.top(),
		radius*2, radius*2,
		16*90, 16*90);

	painter.drawPie(
		rect.right() - radius *2,
		rect.top(),
		radius*2, radius*2,
		0, 16*90);

	painter.drawPie(
		rect.left(),
		rect.bottom() - radius *2,
		radius*2, radius*2,
		16*180, 16*90);

	painter.drawPie(
		rect.right() - radius *2,
		rect.bottom() - radius *2,
		radius*2, radius*2,
		0, -16*90);
}

static void drawRoundRect(QPainter& painter, const QRect& rect, int radius) {
    painter.drawLine(
        rect.left() + radius,
        rect.top(),
        rect.right() - radius,
        rect.top());

    painter.drawLine(
        rect.right() - 1,
        rect.top() + radius,
        rect.right() - 1,
        rect.bottom() - radius);

    painter.drawLine(
        rect.right() - radius,
        rect.bottom() - 1,
        rect.left() + radius,
        rect.bottom() - 1);

    painter.drawLine(
        rect.left(),
        rect.bottom() - radius,
        rect.left(),
        rect.top() + radius);

	painter.drawArc(
		rect.left(),
		rect.top(),
		radius*2, radius*2,
		16*90, 16*90);

	painter.drawArc(
		rect.right() - radius *2,
		rect.top(),
		radius*2, radius*2,
		0, 16*90);

	painter.drawArc(
		rect.left(),
		rect.bottom() - radius *2,
		radius*2, radius*2,
		16*180, 16*90);

	painter.drawArc(
		rect.right() - radius *2,
		rect.bottom() - radius *2,
		radius*2, radius*2,
		0, -16*90);
}


void GVScrollPixmapView::updateFullScreenLabel() {
	QString path=d->mDocument->url().path();	
	QString pathFile=d->mDocument->dirURL().path();
	QString comment=d->mDocument->comment();
    if (comment.isNull()) {
        comment=i18n("(No comment)");
    }
	QString fileName=d->mDocument->filename();
	QString resolution = QString( "%1x%2" ).arg( d->mDocument->width()).arg( d->mDocument->height());

	QString text;
	
	switch (d->mOSDMode) {
	case FREE_OUTPUT: {		
		QString str = d->mFreeOutputFormat;
		str.replace("\\n", "\n");
		QStringList strList = QStringList::split(QRegExp("\%"),str,TRUE);
		for ( QStringList::Iterator it = strList.begin(); it != strList.end(); ++it ) {
			str = *it;
			if ((*it).find('f',0,false) == 0) {
				str = "%" + *it;
				str.replace(QRegExp("\%[fF]"), fileName);
			} else if ((*it).find('c',0,false) == 0) {
				str = "%" + *it;
				str.replace(QRegExp("\%[Cc]"), comment);
			} else if ((*it).find('r',0,false) == 0) {
				str = "%" + *it;
				str.replace(QRegExp("\%[rR]"), resolution);
			} else if ((*it).find('p',0,false) == 0) {
				str = "%" + *it;
				str.replace(QRegExp("\%[pP]"), pathFile);
			}
			text += str;
		}
		break;
	}
	case PATH:
		text = path;
		break;
	case COMMENT:
		text = comment;
		break;
	case PATH_AND_COMMENT:
		text = path + "\n" + comment;
		break;
	case NONE:
		break;
	}
	
	QPainter painter;

	QSize size=d->mFullScreenLabel->fontMetrics().size(0,text);
	size.setWidth( size.width() + FULLSCREEN_LABEL_RADIUS * 2);
	size.setHeight( size.height() + FULLSCREEN_LABEL_RADIUS);
	d->mFullScreenLabel->resize(size);

	// Create a mask for the text
	QBitmap mask(size,true);
	painter.begin(&mask);
	painter.setBrush(Qt::white);
	fillRoundRect(painter, d->mFullScreenLabel->rect(), FULLSCREEN_LABEL_RADIUS);
	painter.end();

	// Draw the text on a pixmap
	QPixmap pixmap(size);
	painter.begin(&pixmap);
	painter.setPen(colorGroup().highlightedText());
	QRect rect=pixmap.rect();
	painter.fillRect(rect, colorGroup().highlight());
	drawRoundRect(painter, rect, FULLSCREEN_LABEL_RADIUS);
	
	rect.addCoords(
        FULLSCREEN_LABEL_RADIUS, FULLSCREEN_LABEL_RADIUS/2,
		-FULLSCREEN_LABEL_RADIUS, -FULLSCREEN_LABEL_RADIUS/2);
	painter.setFont(d->mFullScreenLabel->font());
	
    painter.setPen(black);
	painter.drawText(rect, Qt::DontClip, text);
	
	painter.setPen(colorGroup().highlightedText());
	rect.moveBy(-1, -1);
	painter.drawText(rect, Qt::DontClip, text);
	painter.end();

	// Update the label
	d->mFullScreenLabel->setPixmap(pixmap);
	d->mFullScreenLabel->setMask(mask);
}

void GVScrollPixmapView::updateZoomActions() {
	// Disable most actions if there's no image
	if (d->mDocument->isNull()) {
		d->mZoomIn->setEnabled(false);
		d->mZoomOut->setEnabled(false);
		d->mResetZoom->setEnabled(false);
		return;
	}

	d->mAutoZoom->setEnabled(true);
	d->mResetZoom->setEnabled(true);

	if (d->mAutoZoom->isChecked()) {
		d->mZoomIn->setEnabled(true);
		d->mZoomOut->setEnabled(true);
	} else {
		d->mZoomIn->setEnabled(d->mZoom<MAX_ZOOM);
		d->mZoomOut->setEnabled(d->mZoom>1/MAX_ZOOM);
	}
}


//------------------------------------------------------------------------
//
// File operations
//
//------------------------------------------------------------------------
void GVScrollPixmapView::showFileProperties() {
	(void)new KPropertiesDialog(d->mDocument->url());
}


void GVScrollPixmapView::renameFile() {
	FileOperation::rename(d->mDocument->url(),this);
}


void GVScrollPixmapView::copyFile() {
	KURL::List list;
	list << d->mDocument->url();
	FileOperation::copyTo(list,this);
}


void GVScrollPixmapView::moveFile() {
	KURL::List list;
	list << d->mDocument->url();
	FileOperation::moveTo(list,this);
}


void GVScrollPixmapView::deleteFile() {
	KURL::List list;
	list << d->mDocument->url();
	FileOperation::del(list,this);
}

//------------------------------------------------------------------------
//
// Config
//
//------------------------------------------------------------------------
void GVScrollPixmapView::readConfig(KConfig* config, const QString& group) {
	config->setGroup(group);
	d->mOSDMode=static_cast<OSDMode>(config->readNumEntry(CONFIG_OSD_MODE, static_cast<int>(PATH_AND_COMMENT)) );
	d->mFreeOutputFormat=config->readEntry(CONFIG_FREE_OUTPUT_FORMAT,"%f - %r - %c");
	
	// backwards comp.
	if( config->readEntry(CONFIG_SMOOTH_SCALE) == "true" ) {
		d->mSmoothAlgorithm = GVImageUtils::SMOOTH_NORMAL;
		d->mDelayedSmoothing = true;
	} else {
		int smooth = config->readNumEntry(CONFIG_SMOOTH_SCALE, GVImageUtils::SMOOTH_NORMAL);
		d->mSmoothAlgorithm = static_cast<GVImageUtils::SmoothAlgorithm>(smooth);
		d->mDelayedSmoothing = config->readBoolEntry(CONFIG_DELAYED_SMOOTHING, true);
	}

	d->mEnlargeSmallImages=config->readBoolEntry(CONFIG_ENLARGE_SMALL_IMAGES, false);
	d->mShowScrollBars=config->readBoolEntry(CONFIG_SHOW_SCROLL_BARS, true);
	d->mMouseWheelScroll=config->readBoolEntry(CONFIG_MOUSE_WHEEL_SCROLL, true);
	d->mAutoZoom->setChecked(config->readBoolEntry(CONFIG_AUTO_ZOOM, true));
	updateScrollBarMode();
	d->mLockZoom->setChecked(config->readBoolEntry(CONFIG_LOCK_ZOOM, false));
	d->mBackgroundColor=config->readColorEntry(CONFIG_BACKGROUND_COLOR, &colorGroup().dark());
	if (!d->mFullScreen) {
		viewport()->setBackgroundColor(d->mBackgroundColor);
	}

	d->mMaxRepaintSize = QMIN( LIMIT_MAX_REPAINT_SIZE, QMAX( 10000,
		config->readNumEntry(CONFIG_MAX_REPAINT_SIZE, DEFAULT_MAX_REPAINT_SIZE )));
	d->mMaxScaleRepaintSize = QMIN( LIMIT_MAX_REPAINT_SIZE, QMAX( 10000,
		config->readNumEntry(CONFIG_MAX_SCALE_REPAINT_SIZE, DEFAULT_MAX_REPAINT_SIZE )));
	d->mMaxSmoothRepaintSize = QMIN( LIMIT_MAX_REPAINT_SIZE, QMAX( 10000,
		config->readNumEntry(CONFIG_MAX_SMOOTH_REPAINT_SIZE, DEFAULT_MAX_REPAINT_SIZE )));
}


void GVScrollPixmapView::writeConfig(KConfig* config, const QString& group) const {
	config->setGroup(group);
	config->writeEntry(CONFIG_OSD_MODE, static_cast<int>(d->mOSDMode));
	config->writeEntry(CONFIG_FREE_OUTPUT_FORMAT, d->mFreeOutputFormat);	
	config->writeEntry(CONFIG_SMOOTH_SCALE, d->mSmoothAlgorithm);
	config->writeEntry(CONFIG_DELAYED_SMOOTHING, d->mDelayedSmoothing);
	config->writeEntry(CONFIG_ENLARGE_SMALL_IMAGES, d->mEnlargeSmallImages);
	config->writeEntry(CONFIG_SHOW_SCROLL_BARS, d->mShowScrollBars);
	config->writeEntry(CONFIG_MOUSE_WHEEL_SCROLL, d->mMouseWheelScroll);
	config->writeEntry(CONFIG_AUTO_ZOOM, d->mAutoZoom->isChecked());
	config->writeEntry(CONFIG_LOCK_ZOOM, d->mLockZoom->isChecked());
	config->writeEntry(CONFIG_BACKGROUND_COLOR, d->mBackgroundColor);
	config->writeEntry(CONFIG_MAX_REPAINT_SIZE, d->mMaxRepaintSize);
	config->writeEntry(CONFIG_MAX_SCALE_REPAINT_SIZE, d->mMaxScaleRepaintSize);
	config->writeEntry(CONFIG_MAX_SMOOTH_REPAINT_SIZE, d->mMaxSmoothRepaintSize);
}

