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

#ifndef FEQT_INCLUDED_SRC_manager_UIManagerStyle_h
#define FEQT_INCLUDED_SRC_manager_UIManagerStyle_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QColor>
#include <QIcon>
#include <QPalette>
#include <QString>

/** Shared appearance helpers for the VirtualBox Manager. */
class UIManagerStyle
{
public:

    /** Returns a machine background for @a palette, selection state @a fSelected
      * and hover animation progress @a iHoverIntensity (0 to 100). */
    static QColor machineBackgroundColor(const QPalette &palette, bool fSelected, int iHoverIntensity);

    /** Returns a subtle border color derived from @a palette. */
    static QColor borderColor(const QPalette &palette);

    /** Returns the raised surface color for cards and navigation. */
    static QColor surfaceColor(const QPalette &palette);

    /** Insets details cards from the edges of the machine and phone panes. */
    static constexpr int detailsPaneInset() { return 5; }
    /** Insets the toolbar and content from the navigation edge. */
    static constexpr int pageInset() { return 14; }

    /** Returns the shared background for the pane, navigation and toolbar gutter. */
    static QColor backdropColor(const QPalette &palette);
    /** Returns the style sheet for the Home content. */
    static QString homePaneStyleSheet(const QPalette &palette);
    /** Returns the style sheet for the stacked Manager tool pages. */
    static QString toolPageStyleSheet(const QPalette &palette);
    /** Returns the style sheet for the Manager's main toolbar. */
    static QString toolBarStyleSheet(const QPalette &palette);
    /** Returns the navigation toggle style for @a palette and @a fExpanded. */
    static QString navigationToggleStyleSheet(const QPalette &palette, bool fExpanded);
    /** Draws a navigation panel icon for the given state, layout direction and display scale. */
    static QIcon navigationToggleIcon(const QPalette &palette, bool fExpanded,
                                      Qt::LayoutDirection enmDirection, qreal fDevicePixelRatio);
};

#endif /* !FEQT_INCLUDED_SRC_manager_UIManagerStyle_h */
