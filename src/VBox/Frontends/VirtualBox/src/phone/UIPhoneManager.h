/* $Id$ */
/** @file
 * VBox Qt GUI - Android phone manager dialog.
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

#ifndef FEQT_INCLUDED_SRC_phone_UIPhoneManager_h
#define FEQT_INCLUDED_SRC_phone_UIPhoneManager_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "QIDialog.h"

class QComboBox;
class QAction;
class QFormLayout;
class QFrame;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTabWidget;
class QTableWidget;
class QToolButton;
class QVBoxLayout;
class UIPhoneBackend;

/** Phone creation and runtime controls, independent of the VirtualBox VM API. */
class UIPhoneManager : public QIDialog
{
    Q_OBJECT;

public:
    /** Constructs the manager; an optional data root isolates regression tests. */
    explicit UIPhoneManager(QWidget *pParent = 0, const QString &strDataRoot = QString());
    /** Refreshes the phone list when the dialog is opened again. */
    void refresh();

protected:
    /** Updates all labels after a live language change. */
    virtual void changeEvent(QEvent *pEvent) RT_OVERRIDE;

private:
    void prepareHeader(QVBoxLayout *pLayout);
    void prepareSdk(QVBoxLayout *pLayout);
    void preparePhones();
    void prepareCreation();
    void prepareFooter(QVBoxLayout *pLayout);
    void prepareAppearance();
    void prepareConnections();
    void retranslateUi();
    void updateControlGeometry();
    void updatePreview();
    void showResult(bool fSuccess, const QString &strMessage);
    void applySdk();
    void updateImages();
    void updateDevices();
    void updateControls();
    void createPhone();
    QString selectedId() const;

    UIPhoneBackend *m_pBackend; /**< SDK adapter; remains alive while the dialog is hidden. */
    QLabel *m_pHeading;        /**< Main heading. */
    QLabel *m_pRuntimeState;   /**< Compact SDK setup status. */
    QPushButton *m_pSdkSettings; /**< Expands or collapses the SDK controls. */
    QFrame *m_pSdkGroup;        /**< SDK setup card. */
    QLabel *m_pSdkTitle;        /**< SDK setup heading. */
    QLineEdit *m_pSdkPath;      /**< Explicit SDK selection. */
    QLineEdit *m_pJavaHome;     /**< Explicit Java home selection. */
    QFormLayout *m_pSdkForm;    /**< SDK field labels. */
    QPushButton *m_pBrowseSdk;  /**< SDK directory chooser. */
    QPushButton *m_pBrowseJava; /**< Java directory chooser. */
    QPushButton *m_pApply;      /**< SDK validation and save action. */
    QPushButton *m_pCheck;      /**< Hardware acceleration check. */
    QPushButton *m_pHelp;       /**< Official SDK setup documentation. */
    QTabWidget *m_pTabs;        /**< Running phones and creation pages. */
    QTableWidget *m_pTable;     /**< Catalog with verified runtime status. */
    QLabel *m_pEmpty;           /**< First-use explanation. */
    QLabel *m_pEmptyTitle;      /**< First-use heading. */
    QWidget *m_pEmptyCard;      /**< Shown instead of an empty table. */
    QPushButton *m_pEmptyAction; /**< Opens SDK settings or the creation tab. */
    QWidget *m_pPhoneActions;   /**< Actions for a populated catalog. */
    QPushButton *m_pRefresh;    /**< Explicit runtime refresh. */
    QPushButton *m_pStart;      /**< Normal startup, allowing Quick Boot. */
    QAction *m_pColdBoot;       /**< Startup without loading a snapshot. */
    QPushButton *m_pStop;       /**< Targeted emulator shutdown. */
    QPushButton *m_pInstall;    /**< Targeted APK installation. */
    QToolButton *m_pMore;       /**< Secondary phone actions. */
    QAction *m_pBack;           /**< Android Back key. */
    QAction *m_pHome;           /**< Android Home key. */
    QAction *m_pRecent;         /**< Android recent-apps key. */
    QAction *m_pLog;            /**< Opens the selected phone's startup log. */
    QFormLayout *m_pCreateForm; /**< Creation field labels. */
    QLineEdit *m_pName;         /**< Unicode display name. */
    QComboBox *m_pImage;        /**< Installed compatible system images. */
    QSpinBox *m_pMemory;        /**< Guest RAM. */
    QSpinBox *m_pCpus;          /**< Guest CPU count. */
    QComboBox *m_pStorage;      /**< Data partition capacity. */
    QComboBox *m_pDisplay;      /**< Portrait display preset. */
    QLabel *m_pCreateTitle;     /**< Creation card heading. */
    QLabel *m_pPreviewTitle;    /**< Preview card heading. */
    QLabel *m_pPreviewScreen;   /**< Preview screen label. */
    QLabel *m_pPreviewDisplay;  /**< Selected portrait resolution. */
    QLabel *m_pPreviewMemory;   /**< Summary of the selected resources. */
    QLabel *m_pCreateHelp;      /**< Dependency and lifecycle explanation. */
    QPushButton *m_pCreate;     /**< Creates a private Android AVD. */
    QPlainTextEdit *m_pResult;  /**< Plain-text, bounded tool diagnostics. */
    QLabel *m_pNotice;          /**< One-line operation summary. */
    QPushButton *m_pDetails;    /**< Expands the diagnostic output. */
    QPushButton *m_pClose;      /**< Hides the manager without stopping phones. */
    int m_iSdkCollapsedHeight; /**< Restores the dialog height after hiding SDK settings. */
};

#endif /* !FEQT_INCLUDED_SRC_phone_UIPhoneManager_h */
