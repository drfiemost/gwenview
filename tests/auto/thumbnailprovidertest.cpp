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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "moc_thumbnailprovidertest.cpp"

// Qt
#include <QDir>
#include <QFile>
#include <QImage>
#include <QPainter>

// KDE
#include <qtest_kde.h>
#include <KDebug>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>

// Local
#include "../lib/imageformats/imageformats.h"
#include "../lib/thumbnailprovider/thumbnailprovider.h"
#include "testutils.h"

// libc
#include <errno.h>
#include <string.h>

using namespace Gwenview;

QTEST_KDEMAIN(ThumbnailProviderTest, GUI)

SandBox::SandBox()
: mPath(QDir::currentPath() + "/sandbox")
{}

void SandBox::initDir()
{
    KIO::Job* job;
    QDir dir(mPath);
    if (dir.exists()) {
        KUrl sandBoxUrl("file://" + mPath);
        job = KIO::del(sandBoxUrl);
        QVERIFY2(job->exec(), "Couldn't delete sandbox");
    }
    dir.mkpath(".");
}

void SandBox::fill()
{
    initDir();
    createTestImage("red.png", 300, 200, Qt::red);
    createTestImage("blue.png", 200, 300, Qt::blue);
    createTestImage("small.png", 50, 50, Qt::green);

    copyTestImage("orient6.jpg", 128, 256);
    copyTestImage("orient6-small.jpg", 32, 64);
}

void SandBox::copyTestImage(const QString& testFileName, int width, int height)
{
    QString testPath = pathForTestFile(testFileName);
    KIO::Job* job = KIO::copy(testPath, KUrl(mPath + '/' + testFileName));
    QVERIFY2(job->exec(), "Couldn't copy test image");
    mSizeHash.insert(testFileName, QSize(width, height));
}

static QImage createColoredImage(int width, int height, const QColor& color)
{
    QImage image(width, height, QImage::Format_RGB32);
    QPainter painter(&image);
    painter.fillRect(image.rect(), color);
    return image;
}

void SandBox::createTestImage(const QString& name, int width, int height, const QColor& color)
{
    QImage image = createColoredImage(width, height, color);
    image.save(mPath + '/' + name, "png");
    mSizeHash.insert(name, QSize(width, height));
}

void ThumbnailProviderTest::initTestCase()
{
    qRegisterMetaType<KFileItem>("KFileItem");
    Gwenview::ImageFormats::registerPlugins();
}

void ThumbnailProviderTest::init()
{
    ThumbnailProvider::setThumbnailBaseDir(mSandBox.mPath + "/thumbnails/");
    mSandBox.fill();
}

static void syncRun(ThumbnailProvider *provider)
{
    QEventLoop loop;
    QObject::connect(provider, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
}

void ThumbnailProviderTest::testLoadLocal()
{
    QDir dir(mSandBox.mPath);

    // Create a list of items which will be thumbnailed
    KFileItemList list;
    Q_FOREACH(const QFileInfo & info, dir.entryInfoList(QDir::Files)) {
        KUrl url("file://" + info.absoluteFilePath());
        KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
        list << item;
    }

    // Generate the thumbnails
    ThumbnailProvider provider;
    provider.setThumbnailGroup(ThumbnailGroup::Normal);
    provider.appendItems(list);
    QSignalSpy spy(&provider, SIGNAL(thumbnailLoaded(KFileItem,QPixmap,QSize,qulonglong)));
    syncRun(&provider);
    while (!ThumbnailProvider::isThumbnailWriterEmpty()) {
        QTest::qWait(100);
    }

    // Check we generated the correct number of thumbnails
    QDir thumbnailDir = ThumbnailProvider::thumbnailBaseDir(ThumbnailGroup::Normal);
    // There should be one file less because small.png is a png and is too
    // small to have a thumbnail
    QStringList entryList = thumbnailDir.entryList(QStringList("*.png"));
    QCOMPARE(entryList.count(), mSandBox.mSizeHash.size() - 1);

    // Check thumbnail keys
    QHash<KFileItem, QHash<QString, QString> > thumbnailHash;
    Q_FOREACH(const QString& name, entryList) {
        QImage thumb;
        QVERIFY(thumb.load(thumbnailDir.filePath(name)));

        KUrl url(thumb.text("Thumb::URI"));
        KFileItem item = list.findByUrl(url);
        QVERIFY(!item.isNull());

        QSize originalSize = mSandBox.mSizeHash.value(item.url().fileName());
        uint mtime = item.time(KFileItem::ModificationTime).toTime_t();

        if (mtime == uint(-1)) {
            // This happens from time to time on build.kde.org, but I haven't
            // been able to reproduce it locally, so let's try to gather more
            // information.
            kWarning() << "mtime == -1 for url" << url << ". This should not happen!";
            kWarning() << "errno:" << errno << "message:" << strerror(errno);
            kWarning() << "QFile::exists(" << url.toLocalFile() << "):" << QFile::exists(url.toLocalFile());
            kWarning() << "Recalculating mtime" << item.time(KFileItem::ModificationTime).toTime_t();
            QFAIL("Invalid time for test KFileItem");
        }

        QCOMPARE(thumb.text("Thumb::Image::Width"), QString::number(originalSize.width()));
        QCOMPARE(thumb.text("Thumb::Image::Height"), QString::number(originalSize.height()));
        QCOMPARE(thumb.text("Thumb::Mimetype"), item.mimetype());
        QCOMPARE(thumb.text("Thumb::Size"), QString::number(item.size()));
        QCOMPARE(thumb.text("Thumb::MTime"), QString::number(mtime));
    }

    // Check what was in the thumbnailLoaded() signals
    QCOMPARE(spy.count(), mSandBox.mSizeHash.size());
    QSignalSpy::ConstIterator it = spy.constBegin(),
                              end = spy.constEnd();
    for (; it != end; ++it) {
        const QVariantList args = *it;
        const KFileItem item = qvariant_cast<KFileItem>(args.at(0));
        const QSize size = args.at(2).toSize();
        const QSize expectedSize = mSandBox.mSizeHash.value(item.url().fileName());
        QCOMPARE(size, expectedSize);
    }
}

void ThumbnailProviderTest::testUseEmbeddedOrNot()
{
    QImage expectedThumbnail;
    QPixmap thumbnailPix;
    SandBox sandBox;
    sandBox.initDir();
    // This image is red (0xfe0000) and 256x128 but contains a white 128x64 thumbnail
    sandBox.copyTestImage("embedded-thumbnail.jpg", 256, 128);

    KFileItemList list;
    KUrl url("file://" + QDir(sandBox.mPath).absoluteFilePath("embedded-thumbnail.jpg"));
    list << KFileItem(KFileItem::Unknown, KFileItem::Unknown, url);

    // Loading a normal thumbnail should bring the white one
    {
        ThumbnailProvider provider;
        provider.setThumbnailGroup(ThumbnailGroup::Normal);
        provider.appendItems(list);
        QSignalSpy spy(&provider, SIGNAL(thumbnailLoaded(KFileItem,QPixmap,QSize,qulonglong)));
        syncRun(&provider);

        QCOMPARE(spy.count(), 1);
        expectedThumbnail = createColoredImage(128, 64, Qt::white);
        thumbnailPix = qvariant_cast<QPixmap>(spy.at(0).at(1));
        QVERIFY(TestUtils::imageCompare(expectedThumbnail, thumbnailPix.toImage()));
    }

    // Loading a large thumbnail should bring the red one
    {
        ThumbnailProvider provider;
        provider.setThumbnailGroup(ThumbnailGroup::Large);
        provider.appendItems(list);
        QSignalSpy spy(&provider, SIGNAL(thumbnailLoaded(KFileItem,QPixmap,QSize,qulonglong)));
        syncRun(&provider);

        QCOMPARE(spy.count(), 1);
        expectedThumbnail = createColoredImage(256, 128, QColor(254, 0, 0));
        thumbnailPix = qvariant_cast<QPixmap>(spy.at(0).at(1));
        QVERIFY(TestUtils::imageCompare(expectedThumbnail, thumbnailPix.toImage()));
    }
}

void ThumbnailProviderTest::testLoadRemote()
{
    KUrl url = setUpRemoteTestDir("test.png");
    if (!url.isValid()) {
        return;
    }
    url.addPath("test.png");

    KFileItemList list;
    KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
    list << item;

    ThumbnailProvider provider;
    provider.setThumbnailGroup(ThumbnailGroup::Normal);
    provider.appendItems(list);
    syncRun(&provider);
    while (!ThumbnailProvider::isThumbnailWriterEmpty()) {
        QTest::qWait(100);
    }

    QDir thumbnailDir = ThumbnailProvider::thumbnailBaseDir(ThumbnailGroup::Normal);
    QStringList entryList = thumbnailDir.entryList(QStringList("*.png"));
    QCOMPARE(entryList.count(), 1);
}

void ThumbnailProviderTest::testRemoveItemsWhileGenerating()
{
    QDir dir(mSandBox.mPath);

    // Create a list of items which will be thumbnailed
    KFileItemList list;
    Q_FOREACH(const QFileInfo & info, dir.entryInfoList(QDir::Files)) {
        KUrl url("file://" + info.absoluteFilePath());
        KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
        list << item;
    }

    // Start generating thumbnails for items
    ThumbnailProvider provider;
    provider.setThumbnailGroup(ThumbnailGroup::Normal);
    provider.appendItems(list);
    QEventLoop loop;
    connect(&provider, SIGNAL(finished()), &loop, SLOT(quit()));

    // Remove items, it should not crash
    provider.removeItems(list);
    loop.exec();
}
