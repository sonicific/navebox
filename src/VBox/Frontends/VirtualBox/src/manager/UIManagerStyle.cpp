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

/* GUI includes: */
#include "UIManagerStyle.h"


/** @page pg_ui_manager_style VirtualBox Manager Appearance
 *
 * Manager-specific colors and widget styling live here.  Colors are derived
 * from the application palette so light, dark and custom system themes retain
 * their text and selection colors.  Layout and interaction stay in each widget.
 */

/* static */
QColor UIManagerStyle::machineBackgroundColor(const QPalette &palette, bool fSelected, int iHoverIntensity)
{
    if (fSelected)
        return palette.color(QPalette::Active, QPalette::Highlight);

    const QColor window = palette.color(QPalette::Active, QPalette::Window);
    const QColor highlight = palette.color(QPalette::Active, QPalette::Highlight);
    /* Blend a small amount of the accent into the surface, including for dark themes. */
    const int iWeight = qBound(0, iHoverIntensity, 100);
    return QColor((window.red()   * (1000 - iWeight) + highlight.red()   * iWeight) / 1000,
                  (window.green() * (1000 - iWeight) + highlight.green() * iWeight) / 1000,
                  (window.blue()  * (1000 - iWeight) + highlight.blue()  * iWeight) / 1000);
}

/* static */
QColor UIManagerStyle::borderColor(const QPalette &palette)
{
    QColor color = palette.color(QPalette::Active, QPalette::WindowText);
    color.setAlpha(26);
    return color;
}

/* static */
QColor UIManagerStyle::surfaceColor(const QPalette &palette)
{
    const QColor base = palette.color(QPalette::Active, QPalette::Base);
    const QColor window = palette.color(QPalette::Active, QPalette::Window);
    return QColor((base.red() * 3 + window.red()) / 4,
                  (base.green() * 3 + window.green()) / 4,
                  (base.blue() * 3 + window.blue()) / 4);
}

/* static */
QString UIManagerStyle::toolBarStyleSheet(const QPalette &palette)
{
    /* Scope all rules to the main toolbar, keeping embedded tools and dialogs native. */
    const QString strSurface = surfaceColor(palette).name();
    const QString strWindow = palette.color(QPalette::Active, QPalette::Window).name();
    const QString strBorder = borderColor(palette).name(QColor::HexArgb);
    return QStringLiteral(
        "QToolBar#managerToolBar {"
        " background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        " stop:0 %1,stop:1 %2); border: none;"
        " border-bottom: 1px solid %3; padding: 4px 8px; spacing: 4px; }"
        "QToolBar#managerToolBar QToolButton {"
        " color: palette(button-text); background: transparent;"
        " border: 1px solid transparent; border-radius: 7px; padding: 4px 8px; }"
        "QToolBar#managerToolBar QToolButton:hover {"
        " background: palette(button); border-color: %3; }"
        "QToolBar#managerToolBar QToolButton:pressed,"
        "QToolBar#managerToolBar QToolButton:checked {"
        " background: palette(highlight); color: palette(highlighted-text); }"
        "QToolBar#managerToolBar QToolButton:focus { border: 1px solid palette(highlight); }"
        "QToolBar#managerToolBar QToolButton:disabled { color: palette(mid); }"
        "QToolBar#managerToolBar QToolButton#qt_toolbar_ext_button { padding: 0px; }"
        "QToolBar#managerToolBar::separator {"
        " background: %3; width: 1px; margin: 6px 4px; }").arg(strSurface, strWindow, strBorder);
}
