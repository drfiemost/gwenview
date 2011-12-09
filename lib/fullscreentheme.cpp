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
// Self
#include "fullscreentheme.h"

// Qt
#include <QDir>
#include <QFile>
#include <QString>

// KDE
#include <KComponentData>
#include <KMacroExpanderBase>
#include <KStandardDirs>
#include <KStringHandler>

// Local
#include <lib/gwenviewconfig.h>

namespace Gwenview
{

static const char* THEME_BASE_DIR = "fullscreenthemes/";

struct FullScreenThemePrivate {
    QString mThemeDir;
    QString mStyleSheet;
};

FullScreenTheme::FullScreenTheme(const QString& themeName)
: d(new FullScreenThemePrivate)
{
    // Get theme dir
    d->mThemeDir = KStandardDirs::locate("appdata", THEME_BASE_DIR + themeName + '/');
    if (d->mThemeDir.isEmpty()) {
        kWarning() << "Couldn't find fullscreen theme" << themeName;
        return;
    }

    // Read css file
    QString styleSheetPath = d->mThemeDir + "/style.css";
    QFile file(styleSheetPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        kWarning() << "Couldn't open" << styleSheetPath;
        return;
    }
    QString styleSheet = QString::fromUtf8(file.readAll());

    d->mStyleSheet = replaceThemeVars(styleSheet);
}

FullScreenTheme::~FullScreenTheme()
{
    delete d;
}

QString FullScreenTheme::styleSheet() const
{
    return d->mStyleSheet;
}

QString FullScreenTheme::replaceThemeVars(const QString& styleSheet)
{
    QHash<QString, QString> macros;
    macros["themeDir"] = d->mThemeDir;
    return KMacroExpander::expandMacros(styleSheet, macros, QLatin1Char('$'));
}

QString FullScreenTheme::currentThemeName()
{
    return GwenviewConfig::fullScreenTheme();
}

void FullScreenTheme::setCurrentThemeName(const QString& name)
{
    GwenviewConfig::setFullScreenTheme(name);
}

static bool themeNameLessThan(const QString& s1, const QString& s2)
{
    return KStringHandler::naturalCompare(s1.toLower(), s2.toLower()) < 0;
}

QStringList FullScreenTheme::themeNameList()
{
    QStringList list;
    const QStringList themeBaseDirs =
        KGlobal::mainComponent().dirs()
        ->findDirs("appdata", THEME_BASE_DIR);
    Q_FOREACH(const QString & themeBaseDir, themeBaseDirs) {
        QDir dir(themeBaseDir);
        list += dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    }
    qSort(list.begin(), list.end(), themeNameLessThan);

    return list;
}

struct RenderInfoSet
{
    QMap<FullScreenTheme::State, FullScreenTheme::RenderInfo> infos;
};

static QLinearGradient createGradient()
{
    QLinearGradient gradient(0, 0, 0, 1.);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    return gradient;
}

FullScreenTheme::RenderInfo FullScreenTheme::renderInfo(FullScreenTheme::WidgetType widget, FullScreenTheme::State state)
{
    static QMap<WidgetType, RenderInfoSet> renderInfoMap;
    if (renderInfoMap.isEmpty()) {
        QLinearGradient gradient;

        // Button, normal
        RenderInfo button;
        button.borderRadius = 5;
        button.borderPen = QPen(QColor::fromHsvF(0, 0, .25, .8));
        gradient = createGradient();
        gradient.setColorAt(0, QColor::fromHsvF(0, 0, .34, .66));
        gradient.setColorAt(0.5, QColor::fromHsvF(0, 0, 0, .66));
        button.bgBrush = gradient;
        button.padding = 6;
        button.textPen = QPen(QColor("#ccc"));
        renderInfoMap[ButtonWidget].infos[NormalState] = button;

        // Button, over
        RenderInfo overButton = button;
        overButton.bgBrush = gradient;
        overButton.borderPen = QPen(QColor("#ccc"));
        renderInfoMap[ButtonWidget].infos[MouseOverState] = overButton;

        // Button, down
        RenderInfo downButton = button;
        gradient = createGradient();
        gradient.setColorAt(0, QColor::fromHsvF(0, 0, .12));
        gradient.setColorAt(0.6, Qt::black);
        downButton.bgBrush = gradient;
        downButton.borderPen = QPen(QColor("#444"));
        renderInfoMap[ButtonWidget].infos[DownState] = downButton;

        // Frame
        RenderInfo frame;
        gradient = createGradient();
        gradient.setColorAt(0, QColor::fromHsvF(0, 0, 0.1, .6));
        gradient.setColorAt(.6, QColor::fromHsvF(0, 0, 0, .6));
        frame.bgBrush = gradient;
        frame.borderPen = QPen(QColor::fromHsvF(0, 0, .4, .6));
        frame.borderRadius = 8;
        frame.textPen = QPen(QColor("#ccc"));
        renderInfoMap[FrameWidget].infos[NormalState] = frame;
    }
    RenderInfo normalInfo = renderInfoMap[widget].infos.value(NormalState);
    if (state == NormalState) {
        return normalInfo;
    } else {
        return renderInfoMap[widget].infos.value(state, normalInfo);
    }
}

} // namespace
