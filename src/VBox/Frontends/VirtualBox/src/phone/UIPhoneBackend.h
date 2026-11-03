/* $Id$ */
/** @file
 * VBox Qt GUI - Android phone SDK integration.
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

#ifndef FEQT_INCLUDED_SRC_phone_UIPhoneBackend_h
#define FEQT_INCLUDED_SRC_phone_UIPhoneBackend_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QElapsedTimer>
#include <QMap>
#include <QObject>
#include <QProcessEnvironment>
#include <QStringList>

/* Other includes: */
#include <functional>

class QProcess;

/** An installed, host-compatible Android phone system image. */
struct UIPhoneImage
{
    QString m_strPackage; /**< SDK package identifier. */
    QString m_strLabel;   /**< Human-readable API, tag and architecture. */
    int m_iApi;           /**< Android API level. */
};

/** Parameters for a newly created phone; no shell command fragments are accepted. */
struct UIPhoneOptions
{
    QString m_strName;       /**< Display name, independent of the generated AVD identifier. */
    QString m_strPackage;    /**< Installed system image package. */
    int m_iMemoryMB = 2048;  /**< Guest RAM in megabytes. */
    int m_cCpus = 2;         /**< Virtual CPU count. */
    int m_iStorageGB = 8;    /**< Maximum writable data partition size. */
    int m_iWidth = 720;      /**< Portrait display width. */
    int m_iHeight = 1280;    /**< Portrait display height. */
    int m_iDensity = 320;    /**< Display density in dots per inch. */
};

/** One phone owned by Navebox and its most recently verified runtime state. */
struct UIPhoneDevice
{
    QString m_strId;       /**< Random, private AVD identifier. */
    QString m_strName;     /**< User-visible name. */
    QString m_strSerial;   /**< Verified emulator ADB serial; never a USB device. */
    QString m_strProblem;  /**< Startup problem, if any. */
    bool m_fBooted = false; /**< Whether Android reports boot completion. */
    bool m_fStarting = false; /**< Whether startup is still pending. */
};

/** Asynchronous Android SDK adapter with a separate, private AVD directory.
  * SDK tools and images are supplied by the user; this class never installs
  * packages, accepts licenses, opens a shell, or changes host security settings. */
class UIPhoneBackend : public QObject
{
    Q_OBJECT;

public:
    /** Constructs an adapter. A supplied data directory isolates tests from user data. */
    explicit UIPhoneBackend(QObject *pParent = 0, const QString &strDataRoot = QString());
    /** Cancels an outstanding short-lived tool; detached phones keep running. */
    virtual ~UIPhoneBackend();

    /** Returns the SDK directory saved for this phone catalog. */
    QString sdkRoot() const { return m_strSdkRoot; }
    /** Returns the selected Java home directory. */
    QString javaHome() const { return m_strJavaHome; }
    /** Returns the private directory containing phone data and logs. */
    QString dataRoot() const { return m_strDataRoot; }
    /** Returns installed compatible phone images, highest API first. */
    QList<UIPhoneImage> images() const;
    /** Returns the phone catalog and last verified runtime states. */
    QList<UIPhoneDevice> devices() const { return m_devices.values(); }
    /** Returns whether a foreground SDK operation is in progress. */
    bool isBusy() const { return m_fBusy; }
    /** Returns a suggested SDK directory without executing discovered files. */
    static QString suggestedSdkRoot();
    /** Returns a suggested Java home without executing discovered files. */
    static QString suggestedJavaHome();
    /** Validates and persists an explicitly selected SDK and Java home. */
    bool configure(const QString &strSdkRoot, const QString &strJavaHome, QString &strError);
    /** Checks the selected SDK without modifying its contents. */
    bool isConfigured(QString &strError) const;
    /** Validates phone parameters against the currently installed images. */
    bool validate(const UIPhoneOptions &options, QString &strError) const;
    /** Refreshes runtime identities and boot completion asynchronously. */
    void refresh();
    /** Creates an AVD from an installed image without replacing an existing AVD. */
    void createPhone(const UIPhoneOptions &options);
    /** Checks acceleration, then starts a phone in the emulator's native window. */
    void startPhone(const QString &strId, bool fColdBoot = false);
    /** Requests emulator shutdown after revalidating its AVD identity. */
    void stopPhone(const QString &strId);
    /** Installs one APK on a booted phone after revalidating its identity. */
    void installApk(const QString &strId, const QString &strPath);
    /** Sends a supported navigation key to the selected, verified phone. */
    void sendKey(const QString &strId, int iKey);
    /** Checks whether usable VM acceleration is available. */
    void checkAcceleration();

signals:
    /** Notifies listeners when foreground controls should be disabled or enabled. */
    void sigBusyChanged(bool fBusy);
    /** Notifies listeners after catalog or runtime state changes. */
    void sigDevicesChanged();
    /** Identifies the newly created phone after it is published in the catalog. */
    void sigPhoneCreated(const QString &strId);
    /** Reports completion with plain-text diagnostic output. */
    void sigOperationFinished(bool fSuccess, const QString &strMessage);

private:
    /** Tool completion callback. Output and execution time are bounded. */
    typedef std::function<void (bool, const QString &)> Completion;
    QString toolPath(const QString &strTool) const;
    QString commandLineToolsRoot() const;
    QString avdRoot() const;
    QProcessEnvironment toolEnvironment() const;
    void loadCatalog();
    void setBusy(bool fBusy);
    bool beginOperation();
    void complete(bool fSuccess, const QString &strMessage);
    void runTool(const QString &strProgram, const QStringList &arguments, int cMsTimeout,
                 const Completion &completion, const QByteArray &input = QByteArray());
    void queryNextDevice(QStringList serials);
    void verifiedCommand(const QString &strId, const QStringList &arguments, int cMsTimeout);
    bool writePhoneConfig(const QString &strId, const UIPhoneOptions &options, QString &strError);

    QString m_strDataRoot;                    /**< Absolute private data directory. */
    QString m_strSdkRoot;                     /**< Explicitly selected SDK. */
    QString m_strJavaHome;                    /**< Explicitly selected Java runtime. */
    QMap<QString, UIPhoneDevice> m_devices;   /**< Only phones with valid ownership metadata. */
    QMap<QString, QElapsedTimer> m_starting;  /**< Startup deadlines, independent of ADB enumeration. */
    QProcess *m_pCommand;                     /**< At most one short-lived SDK command. */
    bool m_fBusy;                            /**< Serializes operations and identity checks. */
};

#endif /* !FEQT_INCLUDED_SRC_phone_UIPhoneBackend_h */
