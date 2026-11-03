/* $Id$ */
/** @file
 * VBox Qt GUI - Icon semantics and About dialog regression tests.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

/* Qt includes: */
#include <QAccessible>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QLabel>
#include <QPointer>
#include <QPixmap>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QStyleOptionSpinBox>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTranslator>

/* GUI includes: */
#include "UIAppearance.h"
#include "UIBranding.h"
#include "UIConverter.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UIIconPool.h"
#include "UITranslationEventListener.h"
#include "VBoxAboutDlg.h"

/* Other VBox includes: */
#include <iprt/buildconfig.h>
#include <iprt/test.h>

/** @page pg_tst_navebox_gui Navebox GUI Regression Tests
 *
 * Exercise the installed icon loader and the real About dialog without opening
 * a COM session or modifying VM configuration.  Use the offscreen platform by
 * default; native platform runs skip the clipboard test to preserve user data.
 */

/** Translator that makes every About translation observable without locale files. */
class QITestAboutTranslator : public QTranslator
{
public:
    /** Makes installing this synthetic translator deliver LanguageChange events. */
    virtual bool isEmpty() const RT_OVERRIDE { return false; }
    /** Translates only the About context, preserving placeholders and rich text. */
    virtual QString translate(const char *pszContext, const char *pszSource,
                              const char * /* pszDisambiguation */, int /* cPlural */) const RT_OVERRIDE
    {
        return qstrcmp(pszContext, "VBoxAboutDlg") == 0
             ? QStringLiteral("Translated: ") + QString::fromUtf8(pszSource) : QString();
    }
};

/** Exposes the spinbox style option for subcontrol geometry checks. */
class QITestSpinBox : public QSpinBox
{
public:
    /** Returns the actual option used by the control's style. */
    QStyleOptionSpinBox styleOption() const
    {
        QStyleOptionSpinBox option;
        initStyleOption(&option);
        return option;
    }
};

/** Checks resources and image plugins required by the installed frontend. */
static void tstBrandResources()
{
    RTTestISub("Installed branding resources");
    RTTESTI_CHECK(!UIBranding::icon().pixmap(QSize(64, 64)).isNull());
    for (const QString &strDirection : {QStringLiteral("up"), QStringLiteral("down")})
        for (const QString &strTheme : {QStringLiteral("light"), QStringLiteral("dark")})
            RTTESTI_CHECK(!QPixmap(QStringLiteral(":/navebox-chevron-%1-%2.png").arg(strDirection, strTheme)).isNull());
}

/** Checks the state images used by the VM status bar through the installed loader. */
static void tstMouseStates()
{
    RTTestISub("Mouse capture and integration states");
    const QStringList names = QStringList() << QStringLiteral("mouse_disabled")
                                            << QStringLiteral("mouse")
                                            << QStringLiteral("mouse_can_seamless_uncaptured")
                                            << QStringLiteral("mouse_can_seamless")
                                            << QStringLiteral("mouse_seamless");
    for (int iSize : {10, 16, 20, 24, 32})
        for (int iScale = 1; iScale <= 4; ++iScale)
        {
            QList<QImage> images;
            foreach (const QString &strName, names)
            {
                const QIcon icon = UIIconPool::iconSet(QStringLiteral(":/%1_%2px.png").arg(strName).arg(iSize));
                const QImage image = icon.pixmap(QSize(iSize, iSize), iScale).toImage();
                RTTESTI_CHECK(!image.isNull());
                RTTESTI_CHECK(image.size() == QSize(iSize * iScale, iSize * iScale));
                foreach (const QImage &previous, images)
                    RTTESTI_CHECK(image != previous);
                images.append(image);
            }
        }

    RTTestISub("User-provided icon fallback");
    QTemporaryDir directory;
    RTTESTI_CHECK(directory.isValid());
    if (!directory.isValid())
        return;
    QImage image(16, 16, QImage::Format_ARGB32);
    image.fill(QColor(31, 62, 93));
    const QString strPath = directory.filePath(QStringLiteral("mouse_16px.png"));
    RTTESTI_CHECK(image.save(strPath));
    if (!QFile::exists(strPath))
        return;
    const QImage loaded = UIIconPool::iconSet(strPath).pixmap(16, 16).toImage();
    RTTESTI_CHECK(loaded.size() == image.size());
    RTTESTI_CHECK(loaded.pixelColor(8, 8) == image.pixelColor(8, 8));
}

/** Looks up a rendered label by its visible text. */
static QLabel *tstFindLabel(QWidget &widget, const QString &strText)
{
    foreach (QLabel *pLabel, widget.findChildren<QLabel*>())
        if (pLabel->text() == strText)
            return pLabel;
    return 0;
}

/** Checks translation events, tab state, link recoloring and build-info copying. */
static void tstAbout()
{
    RTTestISub("About language changes");
    VBoxAboutDlg dialog(0, QStringLiteral("7.2.97-test"));
    QTabWidget *pTabs = dialog.findChild<QTabWidget*>(QStringLiteral("aboutTabs"));
    QPushButton *pCopy = dialog.findChild<QPushButton*>(QStringLiteral("copyBuildInformationButton"));
    RTTESTI_CHECK(pTabs && pCopy);
    if (!pTabs || !pCopy)
        return;
    pTabs->setCurrentIndex(1);

    const QStringList messages = QStringList()
        << QStringLiteral("NAVE - The Virtualization Architecture")
        << QStringLiteral("FOUR PILLARS. ONE VIRTUAL WORLD.")
        << QStringLiteral("Network of Worlds") << QStringLiteral("Architecture")
        << QStringLiteral("Virtualization") << QStringLiteral("Efficiency")
        << QStringLiteral("Connect independent virtual machines in one workspace.")
        << QStringLiteral("A flexible, modular foundation for virtual environments.")
        << QStringLiteral("Create, run and manage multiple systems on one computer.")
        << QStringLiteral("Stay in control of compute, memory and storage resources.")
        << QStringLiteral("Contact");
    QList<QPointer<QLabel> > labels;
    foreach (const QString &strMessage, messages)
    {
        QLabel *pLabel = tstFindLabel(dialog, strMessage);
        RTTESTI_CHECK(pLabel);
        labels.append(pLabel);
    }
    QITestAboutTranslator translator;
    RTTESTI_CHECK(qApp->installTranslator(&translator));
    QApplication::processEvents();
    QApplication::processEvents();
    RTTESTI_CHECK(dialog.windowTitle() == QStringLiteral("Translated: About"));
    RTTESTI_CHECK(pTabs->tabText(0) == QStringLiteral("Translated: Overview"));
    RTTESTI_CHECK(pTabs->tabText(1) == QStringLiteral("Translated: Contact && Credits"));
    RTTESTI_CHECK(pCopy->text() == QStringLiteral("Translated: Copy Build Information"));
    RTTESTI_CHECK(pTabs->currentIndex() == 1);
    for (int i = 0; i < labels.size(); ++i)
    {
        RTTESTI_CHECK(!labels.at(i).isNull());
        if (labels.at(i))
            RTTESTI_CHECK(labels.at(i)->text() == QStringLiteral("Translated: ") + messages.at(i));
    }
    RTTESTI_CHECK(qApp->removeTranslator(&translator));
    QApplication::processEvents();
    QApplication::processEvents();
    RTTESTI_CHECK(dialog.windowTitle() == QStringLiteral("About"));
    RTTESTI_CHECK(pTabs->tabText(0) == QStringLiteral("Overview"));
    RTTESTI_CHECK(pTabs->currentIndex() == 1);

    RTTestISub("About link palette changes");
    QLabel *pEmail = 0;
    foreach (QLabel *pLabel, dialog.findChildren<QLabel*>())
        if (pLabel->text().contains(QStringLiteral("mailto:")))
            pEmail = pLabel;
    RTTESTI_CHECK(pEmail);
    QPalette palette = dialog.palette();
    palette.setColor(QPalette::Window, Qt::white);
    palette.setColor(QPalette::Link, QColor(210, 0, 210));
    dialog.setPalette(palette);
    if (pEmail)
        RTTESTI_CHECK(pEmail->text().contains(QStringLiteral("#d200d2")));
    RTTESTI_CHECK(pTabs->currentIndex() == 1);

    RTTestISub("Copy build information");
    if (QGuiApplication::platformName() == QLatin1String("offscreen"))
    {
        pCopy->click();
        const QString strInformation = QApplication::clipboard()->text();
        RTTESTI_CHECK(strInformation.startsWith(UIBranding::name() + QStringLiteral(" 7.2.97-test\n")));
        RTTESTI_CHECK(strInformation.contains(QString::number(RTBldCfgRevision())));
        RTTESTI_CHECK(strInformation.contains(QString::fromLatin1(RTBldCfgType())));
        RTTESTI_CHECK(strInformation.contains(QString::fromLatin1(RTBldCfgTargetDotArch())));
        RTTESTI_CHECK(strInformation.contains(QString::fromLatin1(qVersion())));
        RTTESTI_CHECK(!strInformation.contains(QStringLiteral("%1")));
    }
    else
        RTTestIPrintf(RTTESTLVL_INFO, "Clipboard check skipped on native platforms.\n");
}

/** Checks lifetime of the real dialog and accessible interfaces across repeated openings. */
static void tstAboutLifetime()
{
    RTTestISub("About repeated construction and destruction");
    for (int i = 0; i < 20; ++i)
    {
        QWidget parent;
        QPointer<VBoxAboutDlg> dialog = new VBoxAboutDlg(i % 2 ? &parent : 0);
        QAccessibleInterface *pInterface = QAccessible::queryAccessibleInterface(dialog);
        RTTESTI_CHECK(pInterface);
        const QAccessible::Id uId = pInterface ? QAccessible::uniqueId(pInterface) : 0;
        if (i % 2)
        {
            /* Production dialogs delete themselves on close when owned by a parent. */
            dialog->close();
            QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        }
        else
            delete dialog.data();
        RTTESTI_CHECK(dialog.isNull());
        if (uId)
            RTTESTI_CHECK(!QAccessible::accessibleInterface(uId));
        QApplication::processEvents();
    }
}

/** Checks that styled spinbox buttons do not cover the editable field. */
static void tstControls()
{
    RTTestISub("Styled spinbox geometry");
    for (bool fDark : {false, true})
    {
        QPalette palette = qApp->palette();
        palette.setColor(QPalette::Base, fDark ? QColor(32, 40, 50) : QColor(Qt::white));
        palette.setColor(QPalette::WindowText, fDark ? QColor(Qt::white) : QColor(Qt::black));
        qApp->setPalette(palette);
        qApp->setStyleSheet(UIAppearance::styleSheet(palette));
        for (bool fRtl : {false, true})
        {
            QITestSpinBox spin;
            spin.setLayoutDirection(fRtl ? Qt::RightToLeft : Qt::LeftToRight);
            spin.setRange(0, 999999999);
            spin.setValue(999999999);
            spin.resize(180, 32);
            spin.ensurePolished();
            const QStyleOptionSpinBox option = spin.styleOption();
            const QRect edit = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                           QStyle::SC_SpinBoxEditField, &spin);
            const QRect up = spin.style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, &spin);
            const QRect down = spin.style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxDown, &spin);
            RTTESTI_CHECK(edit.isValid() && up.isValid() && down.isValid());
            RTTESTI_CHECK(!edit.intersects(up) && !edit.intersects(down));
            RTTESTI_CHECK(!up.intersects(down));
        }
    }
}

/** Optionally exports real-dialog renders for visual review. */
static void tstAboutSnapshots()
{
    const QString strDirectory = QString::fromLocal8Bit(qgetenv("VBOX_GUI_TEST_SNAPSHOT_DIR"));
    if (strDirectory.isEmpty())
        return;
    RTTestISub("About visual review snapshots");
    QDir directory(strDirectory);
    RTTESTI_CHECK(directory.exists() || directory.mkpath(QStringLiteral(".")));
    for (bool fDark : {false, true})
    {
        qApp->setStyleSheet(QString());
        QPalette palette = qApp->style()->standardPalette();
        if (fDark)
        {
            palette.setColor(QPalette::Window, QColor(32, 40, 50));
            palette.setColor(QPalette::Base, QColor(38, 48, 62));
            palette.setColor(QPalette::WindowText, Qt::white);
            palette.setColor(QPalette::Text, Qt::white);
            palette.setColor(QPalette::Button, QColor(42, 54, 70));
            palette.setColor(QPalette::ButtonText, Qt::white);
            palette.setColor(QPalette::Link, QColor(125, 185, 245));
        }
        qApp->setPalette(palette);
        qApp->setStyleSheet(UIAppearance::styleSheet(palette));
        VBoxAboutDlg dialog;
        RTTESTI_CHECK(QFontMetrics(dialog.font()).inFont(QChar('N')));
        dialog.show();
        QTabWidget *pTabs = dialog.findChild<QTabWidget*>(QStringLiteral("aboutTabs"));
        if (!pTabs)
        {
            RTTestIFailed("About tabs are missing");
            return;
        }
        for (int iPage = 0; iPage < pTabs->count(); ++iPage)
        {
            pTabs->setCurrentIndex(iPage);
            QApplication::processEvents();
            const QString strName = QStringLiteral("about-%1-%2.png")
                                    .arg(fDark ? QStringLiteral("dark") : QStringLiteral("light")).arg(iPage);
            RTTESTI_CHECK(dialog.grab().save(directory.filePath(strName)));
        }
    }
}

int main(int argc, char **argv)
{
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstNaveboxGui", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("VBOX_DESKTOPWATCHDOGPOLICY_SYNTHTEST", "Disabled");
    QApplication app(argc, argv);
    app.setStyle(QStringLiteral("Fusion"));
    const QString strFontPath = QString::fromLocal8Bit(qgetenv("VBOX_GUI_TEST_FONT"));
    if (!strFontPath.isEmpty())
    {
        const int iFont = QFontDatabase::addApplicationFont(strFontPath);
        RTTESTI_CHECK(iFont >= 0);
        if (iFont >= 0)
            app.setFont(QFont(QFontDatabase::applicationFontFamilies(iFont).value(0), 9));
    }
    UIConverter::create();
    UIDesktopWidgetWatchdog::create();
    UITranslationEventListener::create();

    tstBrandResources();
    tstMouseStates();
    tstAbout();
    tstAboutLifetime();
    tstControls();
    tstAboutSnapshots();

    UITranslationEventListener::destroy();
    UIDesktopWidgetWatchdog::destroy();
    UIConverter::destroy();
    return RTTestSummaryAndDestroy(hTest);
}
