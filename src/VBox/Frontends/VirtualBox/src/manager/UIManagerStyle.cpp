/* $Id$ */
/** @file
 * VBox Qt GUI - VirtualBox Manager appearance helpers.
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
#include <QPainter>
#include <QPixmap>
#include <QtMath>

/* GUI includes: */
#include "UIAppearance.h"
#include "UIManagerStyle.h"


/** @page pg_ui_manager_style VirtualBox Manager Appearance
 *
 * Manager-specific widget styling uses the same palette as the sliding sheets.
 * Layout and interaction stay in each widget.
 */

/** Returns the common fill for Home and the other Manager cards. */
static QString uiManagerCardFill(const UIAppearance::PaneColors &colors)
{
    return colors.fLight
         ? QStringLiteral("qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #f9fafd,stop:1 #fcfcfe)")
         : colors.surface.name();
}

/** Returns the style sheet for scrollbars in a Manager pane. */
static QString uiManagerScrollBarStyleSheet(const UIAppearance::PaneColors &colors)
{
    return QStringLiteral(
        "QScrollBar { background: %1; border: none; border-radius: 6px; margin: 0; }"
        "QScrollBar:vertical { width: 12px; }"
        "QScrollBar:horizontal { height: 12px; }"
        "QScrollBar::handle { background: %2; border-radius: 4px; }"
        "QScrollBar::handle:vertical { min-height: 28px; margin: 2px; }"
        "QScrollBar::handle:horizontal { min-width: 28px; margin: 2px; }"
        "QScrollBar::handle:hover { background: %3; }"
        "QScrollBar::handle:pressed { background: %4; }"
        "QScrollBar::handle:disabled { background: transparent; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; border: none; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }"
        "QAbstractScrollArea::corner { background: %1; }")
        .arg(colors.row.name(), colors.scrollThumb.name(), colors.scrollHover.name(), colors.scrollPressed.name());
}

/** Returns the chevron variant for the Manager's current theme. */
static QString uiManagerChevronTheme(const UIAppearance::PaneColors &colors)
{
    return colors.fLight ? QStringLiteral("light") : QStringLiteral("dark");
}

/* static */
QColor UIManagerStyle::machineBackgroundColor(const QPalette &palette, bool fSelected, int iHoverIntensity)
{
    if (fSelected)
        return palette.color(QPalette::Active, QPalette::Highlight);

    const UIAppearance::PaneColors colors(palette);
    /* Interpolate between the resting surface and the shared hover color. */
    const int iWeight = qBound(0, iHoverIntensity, 100);
    return QColor((colors.surface.red()   * (100 - iWeight) + colors.hover.red()   * iWeight) / 100,
                  (colors.surface.green() * (100 - iWeight) + colors.hover.green() * iWeight) / 100,
                  (colors.surface.blue()  * (100 - iWeight) + colors.hover.blue()  * iWeight) / 100);
}

/* static */
QColor UIManagerStyle::borderColor(const QPalette &palette)
{
    return UIAppearance::PaneColors(palette).border;
}

/* static */
QColor UIManagerStyle::surfaceColor(const QPalette &palette)
{
    return UIAppearance::PaneColors(palette).surface;
}

/* static */
QColor UIManagerStyle::backdropColor(const QPalette &palette)
{
    return UIAppearance::PaneColors(palette).backdrop;
}

/* static */
QString UIManagerStyle::homePaneStyleSheet(const QPalette &palette)
{
    const UIAppearance::PaneColors colors(palette);
    const QString strRowBorder = colors.fLight ? QStringLiteral("#edf0f8") : colors.border.name();

    return QStringLiteral(
        "QWidget#homeContents { background: %1; }"
        "QFrame#homeCard { background: %2; border: 1px solid %3; border-radius: 16px; }"
        "QFrame#homeDivider { background: %4; }"
        "QPushButton#homeAction { background: %5; border: 1px solid %6; border-radius: 11px; }"
        "QPushButton#homeAction:hover { background: %7; }"
        "QPushButton#homeAction:pressed { background: %8; }"
        "QPushButton#homeAction:focus { border-color: palette(window-text); }"
        "QLabel#homePrimaryText { color: %9; }"
        "QLabel#homeSecondaryText { color: %10; }")
        .arg(colors.backdrop.name(), uiManagerCardFill(colors), colors.border.name(), colors.divider.name(), colors.row.name(),
             strRowBorder, colors.hover.name(), colors.pressed.name(), colors.text.name(), colors.secondary.name());
}

/* static */
QString UIManagerStyle::toolPageStyleSheet(const QPalette &palette)
{
    const UIAppearance::PaneColors colors(palette);
    return QStringLiteral(
        "QFrame#managerToolCard { background: %1; border: 1px solid %2; border-radius: 16px; }"
        "QLabel#managerPageIcon { background: %7; border: 1px solid %2; border-radius: 11px; }"
        "QLabel#managerPageTitle { color: %5; font-size: 14pt; font-weight: 600; }"
        "QLabel#managerPageDescription { color: %8; }"
        "QFrame#managerPageDivider { background: %4; }"
        "QFrame#managerToolCard QTreeView, QFrame#managerToolCard QTableView {"
        " background: %6; alternate-background-color: %3; border: 1px solid %2; border-radius: 10px;"
        " gridline-color: %4; selection-background-color: %9; selection-color: %5; }"
        "QFrame#managerToolCard QTreeView::item { padding: 3px 0; }"
        "QFrame#managerToolCard QHeaderView { background: %3; }"
        "QFrame#managerToolCard QHeaderView::section {"
        " background: %3; color: %5; padding: 7px 10px; border: none; border-bottom: 1px solid %4; }"
        "QFrame#managerToolCard QHeaderView::up-arrow, QFrame#managerToolCard QHeaderView::down-arrow {"
        " subcontrol-position: right center; width: 10px; height: 7px; }"
        "QFrame#managerToolCard QHeaderView::up-arrow { image: url(:/navebox-chevron-up-%10.png); }"
        "QFrame#managerToolCard QHeaderView::down-arrow { image: url(:/navebox-chevron-down-%10.png); }"
        "QFrame#managerToolCard QTabWidget::pane { border: 0; }"
        "QFrame#managerToolCard QTabBar::tab:top {"
        " background: %3; border: 1px solid %2; border-radius: 8px;"
        " margin: 0 4px 6px 0; padding: 5px 10px; }"
        "QFrame#managerToolCard QTabBar::tab:top:hover { background: %7; }"
        "QFrame#managerToolCard QTabBar::tab:top:selected { background: %9; color: %5; }"
        "QFrame#managerToolCard QSplitter::handle { background: %4; }")
        .arg(uiManagerCardFill(colors), colors.border.name(), colors.row.name(), colors.divider.name(),
             colors.text.name(), colors.surface.name(), colors.hover.name(), colors.secondary.name(),
             colors.pressed.name(), uiManagerChevronTheme(colors))
        + uiManagerScrollBarStyleSheet(colors);
}

/* static */
QString UIManagerStyle::toolBarStyleSheet(const QPalette &palette)
{
    /* The surface and its contents have separate layout bounds. */
    const UIAppearance::PaneColors colors(palette);
    const QString strSurface = colors.fLight ? QStringLiteral("#f5f8fd") : colors.surface.name();
    const QString strWindow = colors.fLight ? QStringLiteral("#ebf0f8") : colors.backdrop.name();
    const QString strDivider = colors.fLight ? QStringLiteral("#e2e9f4") : colors.divider.name();
    return QStringLiteral(
        "QFrame#managerToolBarSurface {"
        " background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        " stop:0 %1,stop:1 %2); border: none; border-bottom: 1px solid %3; border-radius: 9px; }"
        "QToolBar#managerToolBar { background: transparent; border: none; spacing: 3px; }"
        "QToolBar#managerToolBar QToolButton {"
        " color: palette(button-text); background: transparent;"
        " border: 1px solid transparent; border-radius: 7px; padding: 4px 8px; }"
        "QToolBar#managerToolBar QToolButton:hover:enabled { background: %4; }"
        "QToolBar#managerToolBar QToolButton:pressed,"
        "QToolBar#managerToolBar QToolButton:checked {"
        " background: %5; }"
        "QToolBar#managerToolBar QToolButton:focus { border: 1px solid palette(highlight); }"
        "QToolBar#managerToolBar QToolButton:disabled { color: palette(mid); }"
        "QToolBar#managerToolBar QToolButton[popupMode=\"1\"],"
        "QToolBar#managerToolBar QToolButton[popupMode=\"2\"] { padding: 4px 15px; }"
        "QToolBar#managerToolBar QToolButton::menu-button {"
        " subcontrol-origin: border; subcontrol-position: right center; width: 18px;"
        " background: transparent; border: none; }"
        "QToolBar#managerToolBar QToolButton::menu-indicator,"
        "QToolBar#managerToolBar QToolButton::menu-arrow {"
        " image: url(:/navebox-chevron-down-%6.png); width: 10px; height: 7px; }"
        "QToolBar#managerToolBar QToolButton::menu-indicator {"
        " subcontrol-position: right center; position: relative; right: 3px; }"
        "QToolBar#managerToolBar QToolButton#qt_toolbar_ext_button { padding: 0; min-height: 26px; }"
        "QToolBar#managerToolBar::separator {"
        " background: %3; width: 1px; margin: 6px 4px; }")
        .arg(strSurface, strWindow, strDivider, colors.hover.name(), colors.pressed.name(),
             uiManagerChevronTheme(colors));
}

/* static */
QIcon UIManagerStyle::navigationToggleIcon(const QPalette &palette, bool fExpanded,
                                          Qt::LayoutDirection enmDirection, qreal fDevicePixelRatio)
{
    const int iExtent = qCeil(16 * fDevicePixelRatio);
    QPixmap pixmap(iExtent, iExtent);
    pixmap.setDevicePixelRatio(fDevicePixelRatio);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    const UIAppearance::PaneColors colors(palette);
    if (enmDirection == Qt::RightToLeft)
    {
        painter.translate(16, 0);
        painter.scale(-1, 1);
    }
    const QRectF frame(2, 3, 12, 10);
    if (fExpanded)
    {
        painter.setClipRect(QRectF(2, 3, 4, 10));
        painter.setPen(Qt::NoPen);
        painter.setBrush(colors.pressed);
        painter.drawRoundedRect(frame, 2.5, 2.5);
        painter.setClipping(false);
    }
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(colors.secondary, 1.1));
    painter.drawRoundedRect(frame, 2.5, 2.5);
    painter.drawLine(QPointF(6, 3.4), QPointF(6, 12.6));
    painter.end();
    return QIcon(pixmap);
}
