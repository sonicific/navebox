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

/* Qt includes: */
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QProcess>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSharedPointer>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QSysInfo>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QVersionNumber>

/* Other includes: */
#include <algorithm>

/* GUI includes: */
#include "UIPhoneBackend.h"

/** @page pg_navebox_phone_backend Navebox Android Phone Backend
 *
 * Use the user's installed Android SDK through argument-vector subprocesses.
 * AVDs live in a separate namespace under application data. Commands targeting
 * a running phone verify its random AVD name through the emulator console
 * before sending ADB requests; physical devices are never candidates.
 */

/** Reads a small UTF-8 properties file; refuse oversized or unreadable input. */
static QMap<QString, QString> uiPhoneReadProperties(const QString &strPath)
{
    QMap<QString, QString> values;
    QFile file(strPath);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 1024 * 1024)
        return values;
    const QStringList lines = QString::fromUtf8(file.readAll()).split('\n');
    foreach (const QString &strLine, lines)
    {
        const int iEquals = strLine.indexOf('=');
        if (iEquals > 0 && !strLine.trimmed().startsWith('#'))
            values.insert(strLine.left(iEquals).trimmed(), strLine.mid(iEquals + 1).trimmed());
    }
    return values;
}

/** Returns whether an identifier belongs to the private, random AVD namespace. */
static bool uiPhoneValidId(const QString &strId)
{
    return QRegularExpression(QStringLiteral("\\Anavebox_[0-9a-f]{32}\\z")).match(strId).hasMatch();
}

/** Returns whether a resolved file or directory remains beneath its intended root. */
static bool uiPhoneIsWithin(const QString &strPath, const QString &strRoot)
{
    const QString strCanonical = QFileInfo(strPath).canonicalFilePath();
    const QString strCanonicalRoot = QFileInfo(strRoot).canonicalFilePath();
    if (strCanonical.isEmpty() || strCanonicalRoot.isEmpty())
        return false;
    const QString strRelative = QDir(strCanonicalRoot).relativeFilePath(strCanonical);
    return !QDir::isAbsolutePath(strRelative) && strRelative != ".." && !strRelative.startsWith("../");
}

/** Writes UTF-8 data atomically, preserving an existing file on failure. */
static bool uiPhoneWrite(const QString &strPath, const QByteArray &data)
{
    QSaveFile file(strPath);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit();
}

UIPhoneBackend::UIPhoneBackend(QObject *pParent, const QString &strDataRoot)
    : QObject(pParent)
    , m_strDataRoot(QDir::cleanPath(strDataRoot.isEmpty()
                    ? QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/Phones"
                    : QFileInfo(strDataRoot).absoluteFilePath()))
    , m_pCommand(0)
    , m_fBusy(false)
    , m_fRefreshing(false)
{
    QFile settings(m_strDataRoot + "/settings.json");
    if (settings.open(QIODevice::ReadOnly) && settings.size() <= 16384)
    {
        const QJsonObject object = QJsonDocument::fromJson(settings.readAll()).object();
        m_strSdkRoot = object.value("sdkRoot").toString();
        m_strJavaHome = object.value("javaHome").toString();
    }
    loadCatalog();
}

UIPhoneBackend::~UIPhoneBackend()
{
    if (m_pCommand)
    {
        m_pCommand->disconnect(this);
        m_pCommand->kill();
        m_pCommand->waitForFinished(1000);
    }
}

QString UIPhoneBackend::suggestedSdkRoot()
{
    foreach (const QString &strVariable, QStringList() << "ANDROID_HOME" << "ANDROID_SDK_ROOT")
    {
        const QString strPath = qEnvironmentVariable(qPrintable(strVariable));
        if (QFileInfo(strPath).isAbsolute() && QFileInfo(strPath).isDir())
            return QDir::cleanPath(strPath);
    }
#ifdef RT_OS_WINDOWS
    return QDir::fromNativeSeparators(qEnvironmentVariable("LOCALAPPDATA")) + "/Android/Sdk";
#elif defined(RT_OS_DARWIN)
    return QDir::homePath() + "/Library/Android/sdk";
#else
    return QDir::homePath() + "/Android/Sdk";
#endif
}

QString UIPhoneBackend::suggestedJavaHome()
{
    QStringList candidates;
    candidates << qEnvironmentVariable("JAVA_HOME");
#ifdef RT_OS_WINDOWS
    candidates << QDir::fromNativeSeparators(qEnvironmentVariable("ProgramFiles")) + "/Android/Android Studio/jbr";
#elif defined(RT_OS_DARWIN)
    candidates << "/Applications/Android Studio.app/Contents/jbr/Contents/Home";
#else
    candidates << "/opt/android-studio/jbr" << "/usr/lib/jvm/default-java";
#endif
    const QString strJava = QStandardPaths::findExecutable("java");
    if (!strJava.isEmpty())
        candidates << QFileInfo(strJava).dir().absoluteFilePath("..");
    foreach (const QString &strPath, candidates)
        if (QFileInfo(strPath).isAbsolute() && QFileInfo(strPath + "/bin/java"
#ifdef RT_OS_WINDOWS
                                                     ".exe"
#endif
                                                     ).isFile())
            return QFileInfo(strPath).canonicalFilePath();
    return QString();
}

QString UIPhoneBackend::avdRoot() const
{
    return m_strDataRoot + "/avd";
}

QString UIPhoneBackend::toolPath(const QString &strTool) const
{
    const QString strRelative = strTool == "adb" ? "platform-tools/adb" : "emulator/emulator";
    return m_strSdkRoot + '/' + strRelative
#ifdef RT_OS_WINDOWS
         + ".exe"
#endif
         ;
}

QString UIPhoneBackend::commandLineToolsRoot() const
{
    const QDir directory(m_strSdkRoot + "/cmdline-tools");
    QStringList names = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    std::sort(names.begin(), names.end(), [](const QString &strLeft, const QString &strRight)
    {
        return QVersionNumber::fromString(strLeft) > QVersionNumber::fromString(strRight);
    });
    names.removeAll("latest");
    names.prepend("latest");
    foreach (const QString &strName, names)
    {
        const QString strPath = directory.absoluteFilePath(strName);
        if (   uiPhoneIsWithin(strPath, m_strSdkRoot)
            && QFileInfo(strPath + "/lib/avdmanager-classpath.jar").isFile())
            return strPath;
    }
    return QString();
}

bool UIPhoneBackend::isConfigured(QString &strError) const
{
    if (!QFileInfo(m_strSdkRoot).isAbsolute() || !QFileInfo(m_strSdkRoot).isDir())
        strError = tr("Choose an installed Android SDK directory.");
    else if (!QFileInfo(toolPath("emulator")).isExecutable() || !QFileInfo(toolPath("adb")).isExecutable())
        strError = tr("Install Android Emulator and Android SDK Platform-Tools in the selected SDK.");
    else
        return true;
    return false;
}

bool UIPhoneBackend::configure(const QString &strSdkRoot, const QString &strJavaHome, QString &strError)
{
    strError.clear();
    if (m_fBusy && !m_fRefreshing)
    {
        strError = tr("Wait for the current phone operation to finish.");
        return false;
    }
    if (m_fRefreshing)
    {
        cancelRefresh();
        setBusy(false);
    }
    const QString strOldSdk = m_strSdkRoot;
    const QString strOldJava = m_strJavaHome;
    m_strSdkRoot = QFileInfo(strSdkRoot).canonicalFilePath();
    m_strJavaHome = strJavaHome.isEmpty() ? QString() : QFileInfo(strJavaHome).canonicalFilePath();
    if (!strJavaHome.isEmpty() && m_strJavaHome.isEmpty())
        strError = tr("The selected Java home does not exist.");
    if (   !strError.isEmpty() || !isConfigured(strError)
        || !QDir().mkpath(avdRoot())
        || !uiPhoneWrite(m_strDataRoot + "/settings.json",
                         QJsonDocument(QJsonObject{{"sdkRoot", m_strSdkRoot}, {"javaHome", m_strJavaHome}}).toJson()))
    {
        if (strError.isEmpty())
            strError = tr("Cannot save phone settings in %1.").arg(m_strDataRoot);
        m_strSdkRoot = strOldSdk;
        m_strJavaHome = strOldJava;
        return false;
    }
    return true;
}

QList<UIPhoneImage> UIPhoneBackend::images() const
{
    QList<UIPhoneImage> result;
    const QString strAbi = QSysInfo::currentCpuArchitecture() == "arm64" ? "arm64-v8a" : "x86_64";
    const QDir directory(m_strSdkRoot + "/system-images");
    foreach (const QString &strApi, directory.entryList(QStringList("android-*"), QDir::Dirs | QDir::NoDotAndDotDot))
        foreach (const QString &strTag, QStringList() << "google_apis" << "google_apis_playstore" << "default")
        {
            const QString strPath = directory.absoluteFilePath(strApi + '/' + strTag + '/' + strAbi);
            if (!uiPhoneIsWithin(strPath, m_strSdkRoot) || !QFileInfo(strPath + "/system.img").isFile())
                continue;
            const QMap<QString, QString> values = uiPhoneReadProperties(strPath + "/source.properties");
            bool fValidApi = false;
            const int iApi = values.value("AndroidVersion.ApiLevel").toInt(&fValidApi);
            if (   !fValidApi || iApi < 26 || strApi != QString("android-%1").arg(iApi)
                || values.value("SystemImage.Abi") != strAbi || values.value("SystemImage.TagId") != strTag)
                continue;
            UIPhoneImage image;
            image.m_strPackage = QString("system-images;%1;%2;%3").arg(strApi, strTag, strAbi);
            image.m_strLabel = tr("API %1 / %2 / %3").arg(iApi).arg(strTag, strAbi);
            image.m_iApi = iApi;
            result << image;
        }
    std::stable_sort(result.begin(), result.end(), [](const UIPhoneImage &left, const UIPhoneImage &right)
    {
        return left.m_iApi > right.m_iApi;
    });
    return result;
}

/* static */
QList<UIPhoneProfile> UIPhoneBackend::profiles()
{
    return {{"pixel",       720, 1280, 320},
            {"pixel_xl",   1080, 1920, 420},
            {"pixel_2_xl", 1080, 2160, 420}};
}

/** Resolves explicit display settings to a supported hardware definition. */
static const char *uiPhoneProfileFor(const UIPhoneOptions &options)
{
    for (const UIPhoneProfile &profile : UIPhoneBackend::profiles())
        if (   profile.m_iWidth == options.m_iWidth && profile.m_iHeight == options.m_iHeight
            && profile.m_iDensity == options.m_iDensity)
            return profile.m_pszSdkDevice;
    return 0;
}

UIPhoneProfile UIPhoneBackend::chooseProfile() const
{
    QMap<QString, int> counts;
    for (const UIPhoneDevice &device : m_devices)
        ++counts[device.m_strProfile];

    /* Spread automatic selections across the fleet, randomizing ties. */
    QList<UIPhoneProfile> candidates;
    int cFewest = m_devices.size();
    for (const UIPhoneProfile &profile : profiles())
    {
        const int cDevices = counts.value(profile.m_pszSdkDevice);
        if (cDevices < cFewest)
        {
            candidates.clear();
            cFewest = cDevices;
        }
        if (cDevices == cFewest)
            candidates << profile;
    }
    return candidates.at(QRandomGenerator::global()->bounded(int(candidates.size())));
}

bool UIPhoneBackend::validate(const UIPhoneOptions &options, QString &strError) const
{
    if (!isConfigured(strError))
        return false;
    const QString strName = options.m_strName.trimmed();
    if (strName.isEmpty() || strName.size() > 64 || strName.contains(QRegularExpression("[\\x00-\\x1f\\x7f]")))
        strError = tr("Enter a phone name of 1 to 64 characters without control characters.");
    else if (commandLineToolsRoot().isEmpty())
        strError = tr("Install Android SDK Command-Line Tools (latest) to create phones.");
    else if (m_strJavaHome.isEmpty() || !QFileInfo(m_strJavaHome + "/bin/java"
#ifdef RT_OS_WINDOWS
                                                 ".exe"
#endif
                                                 ).isExecutable())
        strError = tr("Choose a compatible standalone JDK or bundled Java home containing bin/java.");
    else if (options.m_iMemoryMB < 1024 || options.m_iMemoryMB > 16384 || options.m_iMemoryMB % 256 != 0)
        strError = tr("Phone memory must be a multiple of 256 MB between 1024 and 16384 MB.");
    else if (options.m_cCpus < 1 || options.m_cCpus > qMax(1, qMin(8, QThread::idealThreadCount())))
        strError = tr("Choose a CPU count supported by this host (at most 8).");
    else if (!QList<int>({8, 16, 32, 64}).contains(options.m_iStorageGB))
        strError = tr("Choose 8, 16, 32 or 64 GB of phone storage.");
    else if (!options.m_fVaryProfile && !uiPhoneProfileFor(options))
        strError = tr("Choose a supported portrait profile.");
    else
    {
        foreach (const UIPhoneImage &image, images())
            if (image.m_strPackage == options.m_strPackage)
                return true;
        strError = tr("Choose an installed Android phone image matching this host's CPU architecture (API 26 or newer).");
    }
    return false;
}

QProcessEnvironment UIPhoneBackend::toolEnvironment() const
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert("ANDROID_HOME", m_strSdkRoot);
    environment.insert("ANDROID_SDK_ROOT", m_strSdkRoot);
    environment.insert("ANDROID_AVD_HOME", avdRoot());
    if (!m_strJavaHome.isEmpty())
        environment.insert("JAVA_HOME", m_strJavaHome);
    return environment;
}

void UIPhoneBackend::loadCatalog()
{
    const QMap<QString, UIPhoneDevice> previous = m_devices;
    m_devices.clear();
    const QDir directory(avdRoot());
    foreach (const QString &strEntry, directory.entryList(QStringList("navebox_*.avd"), QDir::Dirs | QDir::NoDotAndDotDot))
    {
        const QString strId = strEntry.chopped(4);
        const QString strPath = directory.absoluteFilePath(strEntry);
        const QString strLocator = directory.absoluteFilePath(strId + ".ini");
        if (   !uiPhoneValidId(strId) || !uiPhoneIsWithin(strPath, avdRoot())
            || !QFileInfo(strPath + "/config.ini").isFile()
            || !uiPhoneIsWithin(strPath + "/config.ini", strPath)
            || !uiPhoneIsWithin(strLocator, avdRoot()))
            continue;
        const QString strLocatedPath = uiPhoneReadProperties(strLocator).value("path");
        if (   !QFileInfo(strLocatedPath).isAbsolute()
            || QFileInfo(strLocatedPath).canonicalFilePath() != QFileInfo(strPath).canonicalFilePath())
            continue;
        QFile metadata(strPath + "/navebox.json");
        if (   !uiPhoneIsWithin(metadata.fileName(), strPath)
            || !metadata.open(QIODevice::ReadOnly) || metadata.size() > 16384)
            continue;
        const QJsonObject object = QJsonDocument::fromJson(metadata.readAll()).object();
        if (object.value("id").toString() != strId || object.value("version").toInt() != 1)
            continue;
        UIPhoneDevice device = previous.value(strId);
        device.m_strId = strId;
        device.m_strName = object.value("name").toString().left(64);
        device.m_strProfile = object.value("profile").toString();
        m_devices.insert(strId, device);
    }
}

void UIPhoneBackend::setBusy(bool fBusy)
{
    m_fBusy = fBusy;
    if (!fBusy)
        m_fRefreshing = false;
    emit sigBusyChanged(fBusy);
}

void UIPhoneBackend::cancelRefresh()
{
    if (m_pCommand)
    {
        m_pCommand->disconnect(this);
        m_pCommand->kill();
        m_pCommand->deleteLater();
        m_pCommand = 0;
    }
    m_refreshDevices.clear();
    m_refreshStarting.clear();
}

bool UIPhoneBackend::beginOperation(bool fRefresh)
{
    if (m_fBusy && (!m_fRefreshing || fRefresh))
        return false;
    QString strError;
    if (!isConfigured(strError))
    {
        emit sigOperationFinished(false, strError);
        return false;
    }
    if (m_fRefreshing)
        cancelRefresh();
    m_fRefreshing = fRefresh;
    setBusy(true);
    return true;
}

void UIPhoneBackend::complete(bool fSuccess, const QString &strMessage)
{
    setBusy(false);
    emit sigDevicesChanged();
    emit sigOperationFinished(fSuccess, strMessage);
}

void UIPhoneBackend::runTool(const QString &strProgram, const QStringList &arguments, int cMsTimeout,
                           const Completion &completion, const QByteArray &input)
{
    QProcess *pProcess = new QProcess(this);
    m_pCommand = pProcess;
    pProcess->setProgram(strProgram);
    pProcess->setArguments(arguments);
    pProcess->setProcessEnvironment(toolEnvironment());
    pProcess->setWorkingDirectory(m_strSdkRoot);
    pProcess->setProcessChannelMode(QProcess::MergedChannels);
    QTimer *pTimer = new QTimer(pProcess);
    pTimer->setSingleShot(true);
    const QSharedPointer<QByteArray> pOutput(new QByteArray);
    const QSharedPointer<bool> pLimitExceeded(new bool(false));
    const auto finish = [this, pProcess, pTimer, pOutput, pLimitExceeded, completion](bool fSuccess, const QString &strReason)
    {
        pTimer->stop();
        pProcess->disconnect(this);
        QString strOutput = QString::fromUtf8(*pOutput).trimmed();
        if (!strReason.isEmpty())
            strOutput = strReason + '\n' + strOutput;
        else if (*pLimitExceeded)
            strOutput = tr("SDK tool exceeded its time/output limit.") + '\n' + strOutput;
        m_pCommand = 0;
        pProcess->deleteLater();
        completion(fSuccess && !*pLimitExceeded, strOutput);
    };
    connect(pProcess, &QProcess::readyReadStandardOutput, this, [pProcess, pOutput, pLimitExceeded]()
    {
        pOutput->append(pProcess->readAllStandardOutput());
        if (pOutput->size() > 65536)
        {
            *pLimitExceeded = true;
            pOutput->truncate(65536);
            pProcess->kill();
        }
    });
    connect(pProcess, &QProcess::started, this, [pProcess, input]()
    {
        if (!input.isEmpty())
            pProcess->write(input);
        pProcess->closeWriteChannel();
    });
    connect(pTimer, &QTimer::timeout, this, [pProcess, pLimitExceeded]()
    {
        *pLimitExceeded = true;
        pProcess->kill();
    });
    connect(pProcess, &QProcess::errorOccurred, this, [finish](QProcess::ProcessError enmError)
    {
        if (enmError == QProcess::FailedToStart)
            finish(false, tr("Could not start the SDK tool. Check its installation and permissions."));
    });
    connect(pProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [finish, pProcess, pOutput](int iCode, QProcess::ExitStatus enmStatus)
    {
        pOutput->append(pProcess->readAllStandardOutput().left(qMax(0, 65536 - pOutput->size())));
        const bool fSuccess = enmStatus == QProcess::NormalExit && iCode == 0;
        finish(fSuccess, fSuccess ? QString() : tr("SDK tool failed or exceeded its time/output limit (exit %1).").arg(iCode));
    });
    pTimer->start(cMsTimeout);
    pProcess->start();
}

bool UIPhoneBackend::writePhoneConfig(const QString &strId, const UIPhoneOptions &options, QString &strError)
{
    const QString strDirectory = avdRoot() + '/' + strId + ".avd";
    const QString strConfig = strDirectory + "/config.ini";
    if (   !uiPhoneIsWithin(strDirectory, avdRoot()) || !uiPhoneIsWithin(strConfig, strDirectory)
        || QFileInfo(strConfig).isSymLink())
    {
        strError = tr("The Android tools did not create a valid private AVD directory.");
        return false;
    }
    QMap<QString, QString> values = uiPhoneReadProperties(strConfig);
    if (values.isEmpty())
    {
        strError = tr("Cannot read the newly created Android phone configuration.");
        return false;
    }
    values.insert("avd.ini.displayname", options.m_strName.trimmed());
    values.insert("hw.ramSize", QString::number(options.m_iMemoryMB));
    values.insert("hw.cpu.ncore", QString::number(options.m_cCpus));
    values.insert("disk.dataPartition.size", QString::number(options.m_iStorageGB) + 'G');
    values.insert("hw.lcd.width", QString::number(options.m_iWidth));
    values.insert("hw.lcd.height", QString::number(options.m_iHeight));
    values.insert("hw.lcd.density", QString::number(options.m_iDensity));
    values.insert("skin.name", QString("%1x%2").arg(options.m_iWidth).arg(options.m_iHeight));
    values.insert("skin.dynamic", "yes");
    values.insert("skin.path", "_no_skin");
    values.remove("skin.path.backup");
    values.insert("showDeviceFrame", "no");
    values.insert("hw.keyboard", "yes");
    values.insert("hw.mainKeys", "no");
    values.insert("hw.gpu.enabled", "yes");
    values.insert("hw.gpu.mode", "auto");
    QByteArray data;
    for (auto it = values.cbegin(); it != values.cend(); ++it)
        data += (it.key() + '=' + it.value() + '\n').toUtf8();
    const QJsonObject metadata{{"version", 1}, {"id", strId}, {"name", options.m_strName.trimmed()},
                               {"image", options.m_strPackage}, {"profile", uiPhoneProfileFor(options)}};
    if (!uiPhoneWrite(strConfig, data) || !uiPhoneWrite(strDirectory + "/navebox.json", QJsonDocument(metadata).toJson()))
    {
        strError = tr("Cannot save the new phone configuration. Check free disk space and directory permissions.");
        return false;
    }
    return true;
}

void UIPhoneBackend::createPhone(const UIPhoneOptions &options)
{
    if (!beginOperation())
        return;
    QString strError;
    if (!validate(options, strError))
    {
        complete(false, strError);
        return;
    }
    QStorageInfo storage(m_strDataRoot);
    if (storage.isValid() && storage.bytesAvailable() < qint64(4) * 1024 * 1024 * 1024)
    {
        complete(false, tr("At least 4 GB of free disk space is required to create a phone."));
        return;
    }
    UIPhoneOptions resolved = options;
    if (options.m_fVaryProfile)
    {
        loadCatalog();
        const UIPhoneProfile profile = chooseProfile();
        resolved.m_iWidth = profile.m_iWidth;
        resolved.m_iHeight = profile.m_iHeight;
        resolved.m_iDensity = profile.m_iDensity;
    }
    const QString strId = "navebox_" + QUuid::createUuid().toString(QUuid::Id128);
    const QString strTools = commandLineToolsRoot();
    const QString strJava = m_strJavaHome + "/bin/java"
#ifdef RT_OS_WINDOWS
                         + ".exe"
#endif
                         ;
    /* Invoke the SDK's Java entry point directly: batch-file quoting must never
     * turn SDK paths or package identifiers into shell input on Windows. */
    const QStringList arguments = QStringList()
        << "-Dcom.android.sdkmanager.toolsdir=" + strTools
        << "-cp" << strTools + "/lib/avdmanager-classpath.jar" << "com.android.sdklib.tool.AvdManagerCli"
        << "create" << "avd" << "--name" << strId << "--package" << options.m_strPackage
        << "--device" << uiPhoneProfileFor(resolved) << "--path" << avdRoot() + '/' + strId + ".avd";
    runTool(strJava, arguments, 120000, [this, strId, resolved](bool fSuccess, const QString &strOutput)
    {
        QString strError;
        if (fSuccess)
            fSuccess = writePhoneConfig(strId, resolved, strError);
        loadCatalog();
        if (fSuccess && !m_devices.contains(strId))
        {
            fSuccess = false;
            strError = tr("The created phone could not be loaded from the catalog.");
        }
        complete(fSuccess, fSuccess ? tr("Phone created. Click Start to open it.")
                                   : tr("Phone creation failed. %1\n%2").arg(strError, strOutput));
        if (fSuccess)
            emit sigPhoneCreated(strId);
    }, "no\n");
}

void UIPhoneBackend::refresh()
{
    if (!beginOperation(true))
        return;
    loadCatalog();
    runTool(toolPath("adb"), QStringList() << "devices", 15000, [this](bool fSuccess, const QString &strOutput)
    {
        if (!fSuccess)
        {
            complete(false, tr("Cannot query running phones. %1").arg(strOutput));
            return;
        }
        m_refreshDevices = m_devices;
        m_refreshStarting = m_starting;
        for (auto it = m_refreshDevices.begin(); it != m_refreshDevices.end(); ++it)
        {
            it->m_strSerial.clear();
            it->m_fBooted = false;
            it->m_fStarting = m_refreshStarting.contains(it.key());
            if (it->m_fStarting && m_refreshStarting.value(it.key()).elapsed() > 300000)
            {
                it->m_fStarting = false;
                it->m_strProblem = tr("Boot timed out. Check the phone log. If graphics failed, try Cold Boot with Software Graphics.");
                m_refreshStarting.remove(it.key());
            }
        }
        QStringList serials;
        foreach (const QString &strLine, strOutput.split('\n'))
        {
            const QRegularExpressionMatch match = QRegularExpression("\\A(emulator-([0-9]{4,5}))\\s+device(?:\\s|$)").match(strLine);
            const int iPort = match.captured(2).toInt();
            if (match.hasMatch() && iPort >= 5554 && iPort <= 5682 && iPort % 2 == 0)
                serials << match.captured(1);
        }
        queryNextDevice(serials);
    });
}

void UIPhoneBackend::queryNextDevice(QStringList serials)
{
    if (serials.isEmpty())
    {
        m_devices = std::move(m_refreshDevices);
        m_starting = std::move(m_refreshStarting);
        setBusy(false);
        emit sigDevicesChanged();
        return;
    }
    const QString strSerial = serials.takeFirst();
    runTool(toolPath("adb"), QStringList() << "-s" << strSerial << "emu" << "avd" << "name", 5000,
            [this, strSerial, serials](bool fSuccess, const QString &strOutput)
    {
        const QString strId = strOutput.section('\n', 0, 0).trimmed();
        if (!fSuccess || !m_refreshDevices.contains(strId))
        {
            queryNextDevice(serials);
            return;
        }
        UIPhoneDevice &device = m_refreshDevices[strId];
        device.m_strSerial = strSerial;
        device.m_fStarting = device.m_strProblem.isEmpty();
        if (device.m_fStarting && !m_refreshStarting.contains(strId))
            m_refreshStarting[strId].start();
        runTool(toolPath("adb"), QStringList() << "-s" << strSerial << "shell" << "getprop" << "sys.boot_completed", 5000,
                [this, strId, serials](bool fBootQuery, const QString &strBootOutput)
        {
            UIPhoneDevice &device = m_refreshDevices[strId];
            device.m_fBooted = fBootQuery && strBootOutput.trimmed() == "1";
            if (device.m_fBooted)
            {
                device.m_fStarting = false;
                device.m_strProblem.clear();
                m_refreshStarting.remove(strId);
            }
            queryNextDevice(serials);
        });
    });
}

void UIPhoneBackend::checkAcceleration()
{
    if (beginOperation())
        runTool(toolPath("emulator"), QStringList("-accel-check"), 15000,
                [this](bool fSuccess, const QString &strOutput)
        {
            complete(fSuccess, strOutput);
        });
}

void UIPhoneBackend::startPhone(const QString &strId, bool fColdBoot, bool fSoftwareGraphics)
{
    if (!beginOperation())
        return;
    loadCatalog();
    if (   !m_devices.contains(strId) || !m_devices.value(strId).m_strSerial.isEmpty()
        || m_starting.contains(strId))
    {
        complete(false, tr("Choose a stopped phone. Refresh its status before starting it again."));
        return;
    }
    runTool(toolPath("emulator"), QStringList("-accel-check"), 15000,
            [this, strId, fColdBoot, fSoftwareGraphics](bool fSuccess, const QString &strOutput)
    {
        if (!fSuccess)
        {
            complete(false, tr("Hardware acceleration is unavailable. %1").arg(strOutput));
            return;
        }
        QLockFile lock(m_strDataRoot + '/' + strId + ".start.lock");
        if (!lock.tryLock())
        {
            complete(false, tr("Another Navebox instance is starting this phone."));
            return;
        }
        QProcess process;
        process.setProgram(toolPath("emulator"));
        const QString strGraphics = fSoftwareGraphics ? QStringLiteral("software") : QStringLiteral("auto");
        QStringList arguments = QStringList() << "-avd" << strId << "-accel" << "on" << "-gpu" << strGraphics;
        if (fColdBoot || fSoftwareGraphics)
            arguments << "-no-snapshot-load";
        if (fSoftwareGraphics)
            arguments << "-no-snapshot-save";
        process.setArguments(arguments);
        process.setProcessEnvironment(toolEnvironment());
        process.setWorkingDirectory(m_strSdkRoot);
        process.setStandardInputFile(QProcess::nullDevice());
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.setStandardOutputFile(m_strDataRoot + '/' + strId + ".log", QIODevice::Truncate);
        if (!process.startDetached())
        {
            complete(false, tr("Could not start Android Emulator. Check the SDK installation."));
            return;
        }
        m_starting[strId].start();
        m_devices[strId].m_fStarting = true;
        m_devices[strId].m_strProblem.clear();
        complete(true, tr("Phone is starting in its own window. Its status will change to Ready when Android finishes booting."));
    });
}

void UIPhoneBackend::verifiedCommand(const QString &strId, const QStringList &arguments, int cMsTimeout)
{
    if (!beginOperation())
        return;
    const UIPhoneDevice device = m_devices.value(strId);
    if (!uiPhoneValidId(strId) || device.m_strSerial.isEmpty())
    {
        complete(false, tr("The selected phone is not connected. Refresh its status."));
        return;
    }
    runTool(toolPath("adb"), QStringList() << "-s" << device.m_strSerial << "emu" << "avd" << "name", 5000,
            [this, strId, device, arguments, cMsTimeout](bool fSuccess, const QString &strOutput)
    {
        if (!fSuccess || strOutput.section('\n', 0, 0).trimmed() != strId)
        {
            m_devices[strId].m_strSerial.clear();
            m_devices[strId].m_fBooted = false;
            complete(false, tr("The phone connection changed. Refresh before trying again."));
            return;
        }
        runTool(toolPath("adb"), QStringList() << "-s" << device.m_strSerial << arguments, cMsTimeout,
                [this, strId, arguments](bool fCommandSuccess, const QString &strCommandOutput)
        {
            if (fCommandSuccess && arguments == (QStringList() << "emu" << "kill"))
            {
                m_devices[strId].m_strSerial.clear();
                m_devices[strId].m_fBooted = false;
                m_devices[strId].m_fStarting = false;
                m_starting.remove(strId);
            }
            complete(fCommandSuccess, strCommandOutput.isEmpty() && fCommandSuccess
                                   ? tr("Phone command completed.") : strCommandOutput);
        });
    });
}

void UIPhoneBackend::stopPhone(const QString &strId)
{
    verifiedCommand(strId, QStringList() << "emu" << "kill", 10000);
}

void UIPhoneBackend::installApk(const QString &strId, const QString &strPath)
{
    const QFileInfo file(strPath);
    if (!m_devices.value(strId).m_fBooted || !file.isFile() || file.suffix().compare("apk", Qt::CaseInsensitive) != 0)
    {
        emit sigOperationFinished(false, tr("Choose an APK file and wait until the selected phone is Ready."));
        return;
    }
    verifiedCommand(strId, QStringList() << "install" << "-r" << file.absoluteFilePath(), 180000);
}

void UIPhoneBackend::sendKey(const QString &strId, int iKey)
{
    if (iKey == 3 || iKey == 4 || iKey == 187)
        verifiedCommand(strId, QStringList() << "shell" << "input" << "keyevent" << QString::number(iKey), 10000);
}
