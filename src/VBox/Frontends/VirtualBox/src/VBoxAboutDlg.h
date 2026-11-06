/* $Id: VBoxAboutDlg.h 113628 2026-03-27 14:36:27Z sergey.dubov@oracle.com $ */
/** @file
 * VBox Qt GUI - VBoxAboutDlg class declaration.
 */

/*
 * Copyright (C) 2006-2026 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_VBoxAboutDlg_h
#define FEQT_INCLUDED_SRC_VBoxAboutDlg_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QDialog>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QLabel;
class QPushButton;
class QVBoxLayout;

/** QDialog extension
  * used to show the About-VirtualBox dialog. */
class SHARED_LIBRARY_STUFF VBoxAboutDlg : public QDialog
{
    Q_OBJECT;

public:

    /** Constructs dialog passing @a pParent to the base-class.
      * @param  strVersion  Brings the version number of VirtualBox. */
    VBoxAboutDlg(QWidget *pParent = 0, const QString &strVersion = QString());

protected:

    /** Handles show @a pEvent. */
    virtual void showEvent(QShowEvent *pEvent) RT_OVERRIDE;
    /** Updates translated text and palette-dependent links. */
    virtual void changeEvent(QEvent *pEvent) RT_OVERRIDE;

private slots:

    /** Handles translation event. */
    void sltRetranslateUI();
    /** Copies version and build provenance for support requests. */
    void sltCopyBuildInformation();

private:

    /** Prepares all. */
    void prepare();
    /** Prepares the scrollable page and its fixed action row. */
    void prepareMainLayout();
    /** Adds the branding and version header. */
    void prepareHero(QWidget *pPage, QVBoxLayout *pLayout);
    /** Adds the four NAVE pillars. */
    void prepareOverview(QWidget *pPage, QVBoxLayout *pLayout);
    /** Adds contact, build information and attribution. */
    void prepareSupport(QWidget *pPage, QVBoxLayout *pLayout);
    /** Adds the copy and close actions. */
    void prepareActions(QVBoxLayout *pLayout);
    /** Returns plain-text version and build provenance, without user or VM data. */
    QString buildInformation() const;

    /** Holds the pseudo-parent widget reference. */
    QWidget *m_pPseudoParent;

    /** Holds whether window is polished. */
    bool  m_fPolished;

    /** Holds the VirtualBox version number. */
    QString  m_strVersion;

    /** Holds the product version in the header. */
    QLabel      *m_pVersion;
    /** Holds the upstream attribution and license link. */
    QLabel      *m_pAttribution;
    /** Holds the overview heading. */
    QLabel      *m_pOverviewHeading;
    /** Holds the overview introduction. */
    QLabel      *m_pOverviewIntroduction;
    /** Holds the translated pillar titles. */
    QLabel      *m_apPillarTitles[4];
    /** Holds the translated pillar descriptions. */
    QLabel      *m_apPillarDescriptions[4];
    /** Holds the contact heading. */
    QLabel      *m_pContactHeading;
    /** Holds the palette-dependent contact link. */
    QLabel      *m_pEmail;
    /** Holds the build status and provenance. */
    QLabel      *m_pBuildInformation;
    /** Holds the support-information copy button. */
    QPushButton *m_pCopyBuildInformation;
};

#endif /* !FEQT_INCLUDED_SRC_VBoxAboutDlg_h */
