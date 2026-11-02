/* $Id$ */
/** @file
 * VBox Qt GUI - Scalable glass icon artwork.
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

#ifndef FEQT_INCLUDED_SRC_globals_UIIconPoolTheme_h
#define FEQT_INCLUDED_SRC_globals_UIIconPoolTheme_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QPixmap>
#include <QString>

/** Resolution-independent application icon artwork, separate from icon loading. */
class UIIconPoolTheme
{
public:

    /** Returns themed artwork for a built-in resource at @a iScale (1 to 4).
      * Returns a null pixmap for branding, guest OS logos, illustrations and
      * external files, which must keep their original artwork. */
    static QPixmap pixmap(const QString &strName, int iScale = 1);
};

#endif /* !FEQT_INCLUDED_SRC_globals_UIIconPoolTheme_h */
