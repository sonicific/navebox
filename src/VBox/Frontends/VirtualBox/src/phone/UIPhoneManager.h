/* $Id$ */
/** @file
 * VBox Qt GUI - Android phone manager page.
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

/* Qt includes: */
#include <QWidget>

/* Other VBox includes: */
#include <iprt/cdefs.h>

class QComboBox;
class QAction;
class QIDialog;
class QFormLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QVBoxLayout;
class UIPhoneBackend;
struct UIPhoneDevice;

/** Phone creation and runtime controls, independent of the VirtualBox VM API. */
class UIPhoneManager : public QWidget
{
    Q_OBJECT;

public:
    /** Constructs the manager; an optional data root isolates regression tests. */
    explicit UIPhoneManager(QWidget *pParent = 0, const QString &strDataRoot = QString());
    /** Returns the page actions for the main window toolbar. */
    QList<QAction*> toolbarActions() const;

protected:
    /** Updates all labels after a live language change. */
    virtual void changeEvent(QEvent *pEvent) RT_OVERRIDE;
    /** Refreshes the phone list when the page becomes visible. */
    virtual void showEvent(QShowEvent *pEvent) RT_OVERRIDE;

private:
    void prepareActions();
    void prepareSdk();
    void preparePhones(QVBoxLayout *pLayout);
    void preparePhoneDetails(QVBoxLayout *pLayout);
    void prepareCreation();
    void prepareStatus(QVBoxLayout *pLayout);
    void prepareAppearance();
    void prepareConnections();
    void retranslateUi();
    void updateControlGeometry();
    void updatePreview();
    void showResult(bool fSuccess, const QString &strMessage);
    void refresh();
    void applySdk();
    void updateImages();
    void updateDevices();
    void updatePhoneDetails();
    QString deviceStatus(const UIPhoneDevice &device) const;
    void showCreateDialog();
    void showSdkSettings();
    void updateControls();
    void createPhone();
    QString selectedId() const;

    bool m_fPrepared = false;  /**< Whether all controls can receive change events. */
    UIPhoneBackend *m_pBackend; /**< SDK adapter; remains alive when navigating away. */
    QAction *m_pCreateAction;   /**< Opens the creation dialog from the toolbar. */
    QAction *m_pSdkAction;      /**< Opens SDK settings from the toolbar. */
    QAction *m_pToolbarSeparator; /**< Separates setup and device actions. */
    QIDialog *m_pCreateDialog;  /**< Window-modal phone creation form. */
    QIDialog *m_pSdkDialog;     /**< Window-modal SDK settings. */
    QLabel *m_pSdkTitle;        /**< SDK setup heading. */
    QLineEdit *m_pSdkPath;      /**< Explicit SDK selection. */
    QLineEdit *m_pJavaHome;     /**< Explicit Java home selection. */
    QFormLayout *m_pSdkForm;    /**< SDK field labels. */
    QPushButton *m_pBrowseSdk;  /**< SDK directory chooser. */
    QPushButton *m_pBrowseJava; /**< Java directory chooser. */
    QPushButton *m_pApply;      /**< SDK validation and save action. */
    QPushButton *m_pCheck;      /**< Hardware acceleration check. */
    QPushButton *m_pHelp;       /**< Official SDK setup documentation. */
    QPushButton *m_pSdkDone;    /**< Closes SDK settings. */
    QListWidget *m_pList;       /**< Phone cards with verified runtime status. */
    QLabel *m_pEmpty;           /**< First-use explanation. */
    QLabel *m_pEmptyTitle;      /**< First-use heading. */
    QWidget *m_pEmptyCard;      /**< Shown when the catalog is empty. */
    QPushButton *m_pEmptyAction; /**< Opens SDK settings or the creation dialog. */
    QWidget *m_pDeviceView;     /**< Split device list and details. */
    QWidget *m_pSelectedDetails; /**< Sections shown for a selected phone. */
    QLabel *m_pNoSelection;     /**< Shown before a phone is selected. */
    QFormLayout *m_pIdentity;   /**< Selected phone identity and status. */
    QFormLayout *m_pHardware;   /**< Selected phone profile, RAM and CPUs. */
    QFormLayout *m_pScreen;     /**< Selected phone display configuration. */
    QFormLayout *m_pDisk;       /**< Selected phone data partition. */
    QAction *m_pRefresh;        /**< Explicit runtime refresh. */
    QAction *m_pStart;          /**< Normal startup, allowing Quick Boot. */
    QAction *m_pColdBoot;       /**< Startup without loading a snapshot. */
    QAction *m_pSoftwareBoot;   /**< Cold boot with a software renderer. */
    QAction *m_pStop;           /**< Targeted emulator shutdown. */
    QAction *m_pInstall;        /**< Targeted APK installation. */
    QAction *m_pMore;           /**< Secondary phone actions menu. */
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
    QLabel *m_pCreateNotice;    /**< Reports a creation failure inside the dialog. */
    QLabel *m_pSdkNotice;       /**< Reports SDK validation inside the dialog. */
    QPushButton *m_pCreate;     /**< Creates a private Android AVD. */
    QPushButton *m_pCancel;     /**< Closes the creation form. */
    QPlainTextEdit *m_pResult;  /**< Plain-text, bounded tool diagnostics. */
    QLabel *m_pNotice;          /**< One-line operation summary. */
    QPushButton *m_pDetails;    /**< Expands the diagnostic output. */
};

#endif /* !FEQT_INCLUDED_SRC_phone_UIPhoneManager_h */
