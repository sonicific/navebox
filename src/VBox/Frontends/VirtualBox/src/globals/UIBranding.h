/* $Id$ */
/** @file
 * VBox Qt GUI - Navebox product identity.
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

#ifndef FEQT_INCLUDED_SRC_globals_UIBranding_h
#define FEQT_INCLUDED_SRC_globals_UIBranding_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QIcon>
#include <QString>

/** Product name, also usable in static diagnostic strings. */
#define VBOX_GUI_PRODUCT_NAME "Navebox"
/** Stable Windows taskbar identity, independent of the launching application. */
#define VBOX_GUI_APP_USER_MODEL_ID L"Navebox.Desktop"

/** Shared visible product identity; internal VM and API identifiers stay stable. */
class UIBranding
{
public:
    /** Returns the display name. */
    static QString name() { return QStringLiteral(VBOX_GUI_PRODUCT_NAME); }
    /** Returns the product slogan. */
    static QString slogan() { return QStringLiteral("Command Your Virtual World"); }
    /** Returns the creator's display name. */
    static QString author() { return QStringLiteral("Tran Nguyen Thuong Truong"); }
    /** Returns the contact email address. */
    static QString email() { return QStringLiteral("thuongtruongofficial@gmail.com"); }
    /** Returns the application icon with small and large embedded resolutions. */
    static QIcon icon() { return QIcon(QStringLiteral(":/navebox.ico")); }
};

#endif /* !FEQT_INCLUDED_SRC_globals_UIBranding_h */
