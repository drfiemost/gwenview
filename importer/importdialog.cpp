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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "importdialog.moc"

// Qt
#include <QApplication>
#include <QDate>
#include <QStackedWidget>

// KDE
#include <KDebug>
#include <KGlobalSettings>
#include <KIO/DeleteJob>
#include <KIO/NetAccess>
#include <KLocale>
#include <KMessageBox>
#include <KProtocolInfo>
#include <KRun>
#include <KService>
#include <KStandardGuiItem>
#include <Solid/Device>

// Local
#include "dialogpage.h"
#include "importer.h"
#include "importerconfig.h"
#include "progresspage.h"
#include "thumbnailpage.h"

namespace Gwenview
{

class ImportDialogPrivate
{
public:
    ImportDialog* q;
    QStackedWidget* mCentralWidget;
    ThumbnailPage* mThumbnailPage;
    ProgressPage* mProgressPage;
    DialogPage* mDialogPage;
    Importer* mImporter;

    void deleteImportedUrls()
    {
        KUrl::List importedUrls = mImporter->importedUrlList();
        KUrl::List skippedUrls = mImporter->skippedUrlList();
        int importedCount = importedUrls.count();
        int skippedCount = skippedUrls.count();

        if (importedCount == 0 && skippedCount == 0) {
            return;
        }

        QStringList message;
        message << i18np(
                    "One document has been imported.",
                    "%1 documents have been imported.",
                    importedCount);
        if (skippedCount > 0) {
            message << i18np(
                        "One document has been skipped because it had already been imported.",
                        "%1 documents have been skipped because they had already been imported.",
                        skippedCount);
        }

        if (mImporter->renamedCount() > 0) {
            message[0].append("*");
            message << "<small>* " + i18np(
                        "One of them has been renamed because another document with the same name had already been imported.",
                        "%1 of them have been renamed because other documents with the same name had already been imported.",
                        mImporter->renamedCount())
                    + "</small>";
        }

        message << QString();
        if (skippedCount == 0) {
            message << i18np(
                        "Delete the imported document from the device?",
                        "Delete the %1 imported documents from the device?",
                        importedCount);
        } else if (importedCount == 0) {
            message << i18np(
                        "Delete the skipped document from the device?",
                        "Delete the %1 skipped documents from the device?",
                        skippedCount);
        } else {
            message << i18ncp(
                        "Singular sentence is actually never used.",
                        "Delete the imported or skipped document from the device?",
                        "Delete the %1 imported and skipped documents from the device?",
                        importedCount + skippedCount);
        }

        int answer = KMessageBox::questionYesNo(mCentralWidget,
                                                "<qt>" + message.join("<br/>") + "</qt>",
                                                i18nc("@title:window", "Import Finished"),
                                                KStandardGuiItem::del(),
                                                KGuiItem(i18n("Keep"))
                                               );
        if (answer != KMessageBox::Yes) {
            return;
        }
        KUrl::List urls = importedUrls + skippedUrls;
        while (true) {
            KIO::Job* job = KIO::del(urls);
            if (KIO::NetAccess::synchronousRun(job, q)) {
                break;
            }
            // Deleting failed
            int answer = KMessageBox::warningYesNo(mCentralWidget,
                                                   i18np("Failed to delete the document:\n%2",
                                                           "Failed to delete documents:\n%2",
                                                           urls.count(), job->errorString()),
                                                   QString(),
                                                   KGuiItem(i18n("Retry")),
                                                   KGuiItem(i18n("Ignore"))
                                                  );
            if (answer != KMessageBox::Yes) {
                // Ignore
                break;
            }
        }
    }

    void startGwenview()
    {
        KService::Ptr service = KService::serviceByDesktopName("gwenview");
        if (!service) {
            kError() << "Could not find gwenview";
        } else {
            KRun::run(*service, KUrl::List() << mThumbnailPage->destinationUrl(), 0 /* window */);
        }
    }

    void showWhatNext()
    {
        mCentralWidget->setCurrentWidget(mDialogPage);
        mDialogPage->setText(i18n("What do you want to do now?"));
        mDialogPage->removeButtons();
        int gwenview = mDialogPage->addButton(KGuiItem(i18n("View Imported Documents with Gwenview"), "gwenview"));
        int importMore = mDialogPage->addButton(KGuiItem(i18n("Import more Documents")));
        mDialogPage->addButton(KGuiItem(i18n("Quit"), "dialog-cancel"));

        int answer = mDialogPage->exec();
        if (answer == gwenview) {
            startGwenview();
            qApp->quit();
        } else if (answer == importMore) {
            mCentralWidget->setCurrentWidget(mThumbnailPage);
        } else { /* quit */
            qApp->quit();
        }
    }
};

ImportDialog::ImportDialog()
: d(new ImportDialogPrivate)
{
    d->q = this;
    d->mImporter = new Importer(this);
    connect(d->mImporter, SIGNAL(error(QString)),
            SLOT(showImportError(QString)));
    d->mThumbnailPage = new ThumbnailPage;

    KUrl url = ImporterConfig::destinationUrl();
    if (!url.isValid()) {
        url = KUrl::fromPath(KGlobalSettings::picturesPath());
        int year = QDate::currentDate().year();
        url.addPath(QString::number(year));
    }
    d->mThumbnailPage->setDestinationUrl(url);

    d->mProgressPage = new ProgressPage(d->mImporter);

    d->mDialogPage = new DialogPage;

    d->mCentralWidget = new QStackedWidget;
    setCentralWidget(d->mCentralWidget);
    d->mCentralWidget->addWidget(d->mThumbnailPage);
    d->mCentralWidget->addWidget(d->mProgressPage);
    d->mCentralWidget->addWidget(d->mDialogPage);

    connect(d->mThumbnailPage, SIGNAL(importRequested()),
            SLOT(startImport()));
    connect(d->mThumbnailPage, SIGNAL(rejected()),
            SLOT(close()));
    connect(d->mImporter, SIGNAL(importFinished()),
            SLOT(slotImportFinished()));

    d->mCentralWidget->setCurrentWidget(d->mThumbnailPage);

    setWindowIcon(KIcon("gwenview"));
    setAutoSaveSettings();
}

ImportDialog::~ImportDialog()
{
    delete d;
}

QSize ImportDialog::sizeHint() const
{
    return QSize(700, 500);
}

void ImportDialog::setSourceUrl(const KUrl& url, const QString& deviceUdi)
{
    QString name, iconName;
    if (deviceUdi.isEmpty()) {
        name = url.pathOrUrl();
        iconName = KProtocolInfo::icon(url.protocol());
        if (iconName.isEmpty()) {
            iconName = "folder";
        }
    } else {
        Solid::Device device(deviceUdi);
        name = device.vendor() + " " + device.product();
        iconName = device.icon();
    }
    d->mThumbnailPage->setSourceUrl(url, iconName, name);
}

void ImportDialog::startImport()
{
    KUrl url = d->mThumbnailPage->destinationUrl();
    ImporterConfig::setDestinationUrl(url);
    ImporterConfig::self()->writeConfig();

    d->mCentralWidget->setCurrentWidget(d->mProgressPage);
    d->mImporter->setAutoRenameFormat(
        ImporterConfig::autoRename()
        ? ImporterConfig::autoRenameFormat()
        : QString());
    d->mImporter->start(d->mThumbnailPage->urlList(), url);
}

void ImportDialog::slotImportFinished()
{
    d->deleteImportedUrls();
    d->showWhatNext();
}

void ImportDialog::showImportError(const QString& message)
{
    KMessageBox::sorry(this, message);
    d->mCentralWidget->setCurrentWidget(d->mThumbnailPage);
}

} // namespace
