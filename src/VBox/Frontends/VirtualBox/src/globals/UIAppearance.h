/* $Id$ */
/** @file
 * VBox Qt GUI - Shared palette-aware control appearance.
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

#ifndef FEQT_INCLUDED_SRC_globals_UIAppearance_h
#define FEQT_INCLUDED_SRC_globals_UIAppearance_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <QPalette>
#include <QString>

/** Shared control appearance, independent of product identity and layout. */
class UIAppearance
{
public:
    /** Returns flat, palette-aware input and group-box styling. */
    static QString styleSheet(const QPalette &palette);
};

#endif /* !FEQT_INCLUDED_SRC_globals_UIAppearance_h */
