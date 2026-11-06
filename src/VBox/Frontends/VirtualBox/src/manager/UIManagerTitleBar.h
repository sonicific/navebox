/* $Id$ */
/** @file
 * VBox Qt GUI - Windows Manager title bar.
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


#ifndef FEQT_INCLUDED_SRC_manager_UIManagerTitleBar_h
#define FEQT_INCLUDED_SRC_manager_UIManagerTitleBar_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QCoreApplication>

/* GUI includes: */
#include "UIMenuBar.h"

class QToolButton;

/** Places the Manager menu bar and window controls in the Windows caption. */
class UIManagerTitleBar : public UIMenuBar
{
    Q_DECLARE_TR_FUNCTIONS(UIManagerTitleBar)

public:

    /** Creates a title bar for @a pParent. */
    explicit UIManagerTitleBar(QWidget *pParent);

    /** Extends the Windows client area and adds the caption buttons. */
    void prepareFrame();

    /** Handles native frame messages sent to the Manager. */
    bool handleFrameEvent(const QByteArray &strType, void *pMessage, qintptr *pResult);

protected:

    /** Updates Maximize/Restore when the window state changes. */
    virtual bool eventFilter(QObject *pObject, QEvent *pEvent) RT_OVERRIDE;
    /** Keeps the caption buttons on the right when the language changes direction. */
    virtual void changeEvent(QEvent *pEvent) RT_OVERRIDE;

private:

    /** Matches the menu and caption appearance to the active palette. */
    void updateAppearance();
    /** Places the caption buttons in the menu corner. */
    void updateCaptionCorner();
    /** Sets the caption control labels for the current window state. */
    void retranslateCaptionButtons();

    /** Holds the menu corner widget with the window controls. */
    QWidget *m_pCaption = 0;
    /** Holds the caption buttons, in Minimize, Maximize/Restore, Close order. */
    QToolButton *m_apButtons[3] = {};
    /** Whether native caption replacement is active. */
    bool m_fExtended = false;
};

#endif /* !FEQT_INCLUDED_SRC_manager_UIManagerTitleBar_h */
