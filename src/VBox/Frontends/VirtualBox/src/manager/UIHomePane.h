/* $Id: UIHomePane.h 112403 2026-01-11 19:29:08Z knut.osmundsen@oracle.com $ */
/** @file
 * VBox Qt GUI - UIHomePane class declaration.
 */

/*
 * Copyright (C) 2010-2026 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_manager_UIHomePane_h
#define FEQT_INCLUDED_SRC_manager_UIHomePane_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QIcon>
#include <QList>
#include <QWidget>

/* GUI includes: */
#include "UILibraryDefs.h"
#include "UIManagerDefs.h"

/* Forward declarations: */
class QFrame;
class QLabel;
class QPushButton;
class QVBoxLayout;
class UIHomeActionButton;

/** QWidget subclass holding Home information about VirtualBox. */
class UIHomePane : public QWidget
{
    Q_OBJECT;

signals:

    /** Notify listeners about home @a enmTask was requested. */
    void sigHomeTask(HomeTask enmTask);

public:

    /** Constructs Home pane passing @a pParent to the base-class. */
    UIHomePane(QWidget *pParent = 0);

protected:

    /** Handles any Qt @a pEvent. */
    virtual bool event(QEvent *pEvent) RT_OVERRIDE;

    /** Hides the decorative artwork when the pane is narrow. */
    virtual void resizeEvent(QResizeEvent *pEvent) RT_OVERRIDE;

private slots:

    /** Handles translation event. */
    void sltRetranslateUI();

private:

    /** Prepares all. */
    void prepare();
    /** Adds the branded header to @a pLayout. */
    void prepareHeader(QVBoxLayout *pLayout);
    /** Adds the quick actions to @a pLayout. */
    void prepareActions(QVBoxLayout *pLayout);
    /** Adds the first-run experience mode chooser to @a pLayout. */
    void prepareExperienceMode(QVBoxLayout *pLayout);
    /** Applies and saves the selected experience mode. */
    void chooseExperienceMode(bool fExpertMode);
    /** Updates the panel and row colors to match the current palette. */
    void updateAppearance();
    /** Refreshes the artwork and icons for the current device pixel ratio. */
    void updatePixmaps();

    /** Holds the main Home card. */
    QFrame                   *m_pCard;
    /** Holds the product icon label. */
    QLabel                   *m_pLabelIcon;
    /** Holds the welcome title. */
    QLabel                   *m_pLabelTitle;
    /** Holds the welcome subtitle. */
    QLabel                   *m_pLabelSubtitle;
    /** Holds the decorative banner artwork. */
    QLabel                   *m_pLabelArtwork;
    /** Holds the product icon. */
    QIcon                     m_iconBrand;
    /** Holds the action rows in display order. */
    QList<UIHomeActionButton*> m_actionButtons;
    /** Holds the first-run experience mode section, if needed. */
    QWidget                  *m_pExperienceMode;
    /** Holds the explanation of experience modes. */
    QLabel                   *m_pLabelMode;
    /** Holds the Basic mode button. */
    QPushButton              *m_pButtonBasic;
    /** Holds the Expert mode button. */
    QPushButton              *m_pButtonExpert;
};

#endif /* !FEQT_INCLUDED_SRC_manager_UIHomePane_h */
