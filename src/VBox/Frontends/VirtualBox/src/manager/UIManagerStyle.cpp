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
    color.setAlpha(40);
    return color;
}

/* static */
QString UIManagerStyle::toolBarStyleSheet()
{
    /* Scope all rules to the main toolbar, keeping embedded tools and dialogs native. */
    return QStringLiteral(
        "QToolBar#managerToolBar {"
        " background: palette(window); border: none;"
        " border-bottom: 1px solid palette(mid); padding: 8px; spacing: 6px; }"
        "QToolBar#managerToolBar QToolButton {"
        " color: palette(button-text); background: transparent;"
        " border: 1px solid transparent; border-radius: 6px; padding: 6px 10px; }"
        "QToolBar#managerToolBar QToolButton:hover { background: palette(midlight); }"
        "QToolBar#managerToolBar QToolButton:pressed,"
        "QToolBar#managerToolBar QToolButton:checked {"
        " background: palette(highlight); color: palette(highlighted-text); }"
        "QToolBar#managerToolBar QToolButton:focus { border: 1px solid palette(highlight); }"
        "QToolBar#managerToolBar QToolButton:disabled { color: palette(mid); }"
        "QToolBar#managerToolBar QToolButton#qt_toolbar_ext_button { padding: 0px; }"
        "QToolBar#managerToolBar::separator {"
        " background: palette(mid); width: 1px; margin: 8px 4px; }");
}
