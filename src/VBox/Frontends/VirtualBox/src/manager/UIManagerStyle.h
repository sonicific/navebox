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

    /** Returns the style sheet for the Manager's main toolbar. */
    static QString toolBarStyleSheet();
};

#endif /* !FEQT_INCLUDED_SRC_manager_UIManagerStyle_h */
