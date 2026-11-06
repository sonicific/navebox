/* $Id$ */
/** @file
 * VBox Qt GUI - Android phone subprocess and UI regression tests.
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
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSpinBox>
#include <QStackedWidget>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QTranslator>

/* Other includes: */
#include <cstdio>

/* GUI includes: */
#include "UIAppearance.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UIPhoneBackend.h"
#include "UIPhoneManager.h"

/* Other VBox includes: */
#include <iprt/test.h>

/** @page pg_tst_navebox_phone Android Phone Regression Tests
 *
 * Copies this executable into an isolated mock SDK and exercises real Qt
 * subprocess invocation, argument preservation, failure handling and UI state.
 * No installed SDK, ADB server, VM, license acceptance or user data is involved.
 */

/** Reads a bounded test file. */
static QByteArray tstRead(const QString &strPath)
{
    QFile file(strPath);
    return file.open(QIODevice::ReadOnly) ? file.read(1024 * 1024) : QByteArray();
}

/** Writes test data atomically, also used by detached mock tools. */
static bool tstWrite(const QString &strPath, const QByteArray &data)
{
    QSaveFile file(strPath);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit();
}

/** Changes one mock tool behavior without changing the application environment. */
static void tstControl(const QString &strRoot, const QString &strKey, const QJsonValue &value)
{
    QJsonObject object = QJsonDocument::fromJson(tstRead(strRoot + "/control.json")).object();
    object.insert(strKey, value);
    RTTESTI_CHECK(tstWrite(strRoot + "/control.json", QJsonDocument(object).toJson()));
}

/** Writes stdout from a mock process. */
static void tstOutput(const QByteArray &data)
{
    QFile file;
    if (file.open(stdout, QIODevice::WriteOnly))
        file.write(data);
}

/** Implements mock emulator, adb and java executables using actual child processes. */
static int tstFakeTool(const QString &strTool)
{
    const QString strRoot = qEnvironmentVariable("NAVEBOX_PHONE_FAKE_ROOT");
    const QStringList arguments = QCoreApplication::arguments().mid(1);
    QJsonObject control = QJsonDocument::fromJson(tstRead(strRoot + "/control.json")).object();
    const QJsonObject call{{"tool", strTool}, {"args", QJsonArray::fromStringList(arguments)},
                           {"avdHome", qEnvironmentVariable("ANDROID_AVD_HOME")}};
    QFile calls(strRoot + "/calls.jsonl");
    if (calls.open(QIODevice::WriteOnly | QIODevice::Append))
        calls.write(QJsonDocument(call).toJson(QJsonDocument::Compact) + '\n');
    calls.close();
    if (control.value("delay").toInt() > 0)
        QThread::msleep(control.value("delay").toInt());
    if (control.value("flood").toBool())
    {
        tstOutput(QByteArray(100000, 'x'));
        return 0;
    }
    QJsonObject running = control.value("running").toObject();
    if (strTool == "java")
    {
        if (control.value("createFails").toBool())
        {
            tstOutput("Error: requested SDK image is unavailable\n");
            return 1;
        }
        const QString strId = arguments.value(arguments.indexOf("--name") + 1);
        const QString strPath = arguments.value(arguments.indexOf("--path") + 1);
        if (!QDir().mkpath(strPath))
            return 1;
        if (   !tstWrite(strPath + "/config.ini", "hw.cpu.arch=x86_64\nhw.ramSize=1024\n")
            || !tstWrite(qEnvironmentVariable("ANDROID_AVD_HOME") + '/' + strId + ".ini",
                         ("avd.ini.encoding=UTF-8\npath=" + strPath + '\n').toUtf8()))
            return 1;
        tstOutput("Created AVD\n");
        return 0;
    }
    if (strTool == "emulator")
    {
        if (arguments.contains("-accel-check"))
        {
            const bool fAvailable = control.value("acceleration").toBool(true);
            tstOutput(fAvailable ? "accel:\n0\nHardware acceleration is usable\n" : "Hardware acceleration is unavailable\n");
            return fAvailable ? 0 : 1;
        }
        const QString strId = arguments.value(arguments.indexOf("-avd") + 1);
        running.insert(strId, "emulator-5554");
        control.insert("running", running);
        if (!tstWrite(strRoot + "/control.json", QJsonDocument(control).toJson()))
            return 1;
        tstOutput("Emulator mock started\n");
        return 0;
    }
    if (control.value("adbFails").toBool())
    {
        tstOutput("error: ADB server is unavailable\n");
        return 1;
    }
    if (arguments == QStringList("devices"))
    {
        QByteArray data("List of devices attached\nphysical-phone\tdevice\nemulator-5556\tdevice\n");
        for (auto it = running.constBegin(); it != running.constEnd(); ++it)
            data += it.value().toString().toUtf8() + "\tdevice\n";
        tstOutput(data);
        return 0;
    }
    const QString strSerial = arguments.value(1);
    QString strId = "outside_navebox";
    for (auto it = running.constBegin(); it != running.constEnd(); ++it)
        if (it.value().toString() == strSerial)
            strId = it.key();
    if (arguments.mid(2) == (QStringList() << "emu" << "avd" << "name"))
    {
        tstOutput((control.value("wrongIdentity").toBool() ? QString("different_avd") : strId).toUtf8() + "\nOK\n");
        return 0;
    }
    if (arguments.mid(2) == (QStringList() << "shell" << "getprop" << "sys.boot_completed"))
    {
        tstOutput(control.value("booted").toBool() ? "1\n" : "\n");
        return 0;
    }
    if (arguments.value(2) == "install" && control.value("installFails").toBool())
    {
        tstOutput("Failure [INSTALL_FAILED_NO_MATCHING_ABIS]\n");
        return 1;
    }
    if (arguments.mid(2) == (QStringList() << "emu" << "kill"))
    {
        running.remove(strId);
        control.insert("running", running);
        if (!tstWrite(strRoot + "/control.json", QJsonDocument(control).toJson()))
            return 1;
    }
    tstOutput("Success\n");
    return 0;
}

/** Pumps the UI event loop while waiting for one bounded SDK operation. */
static bool tstWait(UIPhoneBackend &backend, int cMsTimeout = 10000)
{
    QElapsedTimer timer;
    timer.start();
    while (backend.isBusy() && timer.elapsed() < cMsTimeout)
    {
        QApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(1);
    }
    QApplication::processEvents();
    return !backend.isBusy();
}

/** Waits for the detached mock to publish its running state. */
static bool tstWaitStarted(const QString &strRoot)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 5000)
    {
        if (!QJsonDocument::fromJson(tstRead(strRoot + "/control.json")).object().value("running").toObject().isEmpty())
            return true;
        QApplication::processEvents();
        QThread::msleep(5);
    }
    return false;
}

/** Creates executable mock tools in a path that would be dangerous to shell interpolation. */
static bool tstPrepareSdk(const QString &strRoot, QString &strSdk, QString &strJava)
{
    strSdk = strRoot + "/SDK & % ! (" + QChar(0x0111) + "ien thoai)";
    strJava = strRoot + "/Java & runtime";
    QString strSuffix;
#ifdef RT_OS_WINDOWS
    strSuffix = ".exe";
#endif
    const QStringList tools = QStringList() << strSdk + "/emulator/emulator" + strSuffix
                                           << strSdk + "/platform-tools/adb" + strSuffix
                                           << strJava + "/bin/java" + strSuffix;
    foreach (const QString &strTool, tools)
    {
        if (   !QDir().mkpath(QFileInfo(strTool).absolutePath())
            || !QFile::copy(QCoreApplication::applicationFilePath(), strTool))
            return false;
        QFile::setPermissions(strTool, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
#ifdef RT_OS_WINDOWS
        /* Each mock is a real PE executable. Keep its dependencies beside it,
         * also on hosts whose loader does not search PATH for private DLLs. */
        const QDir dependencies(QCoreApplication::applicationDirPath());
        foreach (const QFileInfo &dependency, dependencies.entryInfoList(QStringList("*.dll"), QDir::Files))
            if (!QFile::copy(dependency.absoluteFilePath(), QFileInfo(strTool).dir().filePath(dependency.fileName())))
                return false;
#endif
    }
    const QString strTools = strSdk + "/cmdline-tools/latest/lib";
    const QString strImage = strSdk + "/system-images/android-35/google_apis/"
                           + (QSysInfo::currentCpuArchitecture() == "arm64" ? "arm64-v8a" : "x86_64");
    const QString strAbi = QSysInfo::currentCpuArchitecture() == "arm64" ? "arm64-v8a" : "x86_64";
    return QDir().mkpath(strTools) && tstWrite(strTools + "/avdmanager-classpath.jar", "test fixture")
        && QDir().mkpath(strImage) && tstWrite(strImage + "/system.img", "test fixture")
        && tstWrite(strImage + "/source.properties", ("AndroidVersion.ApiLevel=35\nSystemImage.TagId=google_apis\nSystemImage.Abi="
                                                        + strAbi + '\n').toUtf8());
}

/** Exercises creation, lifecycle, ADB isolation, persistence and subprocess failures. */
static void tstBackend(const QString &strRoot, const QString &strSdk, const QString &strJava)
{
    UIPhoneBackend backend(0, strRoot + "/data");
    bool fSuccess = false;
    QString strMessage;
    QObject::connect(&backend, &UIPhoneBackend::sigOperationFinished, &backend,
                     [&fSuccess, &strMessage](bool fResult, const QString &strText)
    {
        fSuccess = fResult;
        strMessage = strText;
    });
    QString strError;
    RTTestISub("SDK configuration and input validation");
    RTTESTI_CHECK(!backend.isConfigured(strError));
    RTTESTI_CHECK(backend.configure(strSdk, strJava, strError));
    RTTESTI_CHECK(backend.images().size() == 1);
    RTTESTI_CHECK(!backend.configure(strRoot + "/missing-sdk", strJava, strError));
    RTTESTI_CHECK(backend.sdkRoot() == QFileInfo(strSdk).canonicalFilePath());
    RTTESTI_CHECK(!backend.configure(strSdk, strRoot + "/missing-java", strError));
    UIPhoneOptions options;
    options.m_strName = QString("Phone ") + QChar(0x0111) + " & $(echo test) % !";
    options.m_strPackage = "system-images;android-35;google_apis;" +
                          QString(QSysInfo::currentCpuArchitecture() == "arm64" ? "arm64-v8a" : "x86_64");
    options.m_cCpus = 1;
    RTTESTI_CHECK(backend.validate(options, strError));
    UIPhoneOptions invalid = options;
    invalid.m_strName = "injected\nhw.ramSize=1";
    RTTESTI_CHECK(!backend.validate(invalid, strError));
    invalid = options;
    invalid.m_strPackage += ";--force";
    RTTESTI_CHECK(!backend.validate(invalid, strError));
    invalid = options;
    invalid.m_iMemoryMB = 1025;
    RTTESTI_CHECK(!backend.validate(invalid, strError));
    invalid = options;
    invalid.m_cCpus = 0;
    RTTESTI_CHECK(!backend.validate(invalid, strError));
    invalid = options;
    invalid.m_iStorageGB = -1;
    RTTESTI_CHECK(!backend.validate(invalid, strError));
    invalid = options;
    invalid.m_iWidth = 0;
    RTTESTI_CHECK(!backend.validate(invalid, strError));

    RTTestISub("Asynchronous creation and literal subprocess arguments");
    tstControl(strRoot, "delay", 150);
    backend.createPhone(options);
    RTTESTI_CHECK(backend.isBusy());
    int cTicks = 0;
    QTimer heartbeat;
    QObject::connect(&heartbeat, &QTimer::timeout, &backend, [&cTicks]() { ++cTicks; });
    heartbeat.start(10);
    RTTESTI_CHECK(tstWait(backend));
    heartbeat.stop();
    RTTESTI_CHECK(cTicks >= 2);
    RTTESTI_CHECK(fSuccess);
    RTTESTI_CHECK(backend.devices().size() == 1);
    tstControl(strRoot, "delay", 0);
    if (backend.devices().size() != 1)
        return;
    const QString strId = backend.devices().first().m_strId;
    const QString strConfig = strRoot + "/data/avd/" + strId + ".avd/config.ini";
    const QByteArray config = tstRead(strConfig);
    RTTESTI_CHECK(config.contains("hw.ramSize=2048\n"));
    RTTESTI_CHECK(config.contains("disk.dataPartition.size=8G\n"));
    RTTESTI_CHECK(config.contains(options.m_strName.toUtf8()));
    RTTESTI_CHECK(!config.contains("hw.ramSize=1\n"));
    const QByteArray calls = tstRead(strRoot + "/calls.jsonl");
    RTTESTI_CHECK(calls.contains("com.android.sdklib.tool.AvdManagerCli"));
    RTTESTI_CHECK(calls.contains("system-images;android-35;google_apis;"));
    RTTESTI_CHECK(!calls.contains("cmd.exe"));
    UIPhoneBackend restored(0, strRoot + "/data");
    RTTESTI_CHECK(restored.devices().size() == 1);
    RTTESTI_CHECK(restored.devices().first().m_strName == options.m_strName);

    RTTestISub("Acceleration failures and boot lifecycle");
    tstControl(strRoot, "acceleration", false);
    backend.startPhone(strId);
    RTTESTI_CHECK(tstWait(backend));
    RTTESTI_CHECK(!fSuccess);
    RTTESTI_CHECK(!backend.devices().first().m_fStarting);
    tstControl(strRoot, "acceleration", true);
    backend.startPhone(strId, true, true);
    RTTESTI_CHECK(tstWait(backend));
    RTTESTI_CHECK(fSuccess);
    RTTESTI_CHECK(tstWaitStarted(strRoot));
    const QByteArray launch = tstRead(strRoot + "/calls.jsonl");
    RTTESTI_CHECK(launch.contains("\"-gpu\",\"software\",\"-no-snapshot-load\",\"-no-snapshot-save\""));
    backend.refresh();
    RTTESTI_CHECK(tstWait(backend));
    RTTESTI_CHECK(backend.devices().first().m_fStarting);
    RTTESTI_CHECK(!backend.devices().first().m_fBooted);
    RTTESTI_CHECK(backend.devices().first().m_strSerial == "emulator-5554");
    tstControl(strRoot, "booted", true);
    backend.refresh();
    RTTESTI_CHECK(tstWait(backend));
    RTTESTI_CHECK(backend.devices().first().m_fBooted);
    tstControl(strRoot, "delay", 200);
    backend.refresh();
    RTTESTI_CHECK(backend.isRefreshing());
    backend.sendKey(strId, 3);
    RTTESTI_CHECK(!backend.isRefreshing());
    RTTESTI_CHECK(tstWait(backend));
    RTTESTI_CHECK(fSuccess && backend.devices().first().m_fBooted);
    tstControl(strRoot, "delay", 0);
    backend.startPhone(strId);
    RTTESTI_CHECK(!backend.isBusy());
    RTTESTI_CHECK(!fSuccess);

    RTTestISub("APK targeting, command failure and identity changes");
    const QString strApk = strRoot + "/app & % (test).apk";
    RTTESTI_CHECK(tstWrite(strApk, "test fixture"));
    backend.installApk(strId, strApk);
    RTTESTI_CHECK(tstWait(backend));
    RTTESTI_CHECK(fSuccess);
    tstControl(strRoot, "installFails", true);
    backend.installApk(strId, strApk);
    RTTESTI_CHECK(tstWait(backend));
    RTTESTI_CHECK(!fSuccess);
    RTTESTI_CHECK(strMessage.contains("INSTALL_FAILED_NO_MATCHING_ABIS"));
    tstControl(strRoot, "installFails", false);
    backend.sendKey(strId, 3);
    RTTESTI_CHECK(tstWait(backend));
    RTTESTI_CHECK(fSuccess);
    const QByteArray before = tstRead(strRoot + "/calls.jsonl");
    tstControl(strRoot, "wrongIdentity", true);
    backend.stopPhone(strId);
    RTTESTI_CHECK(tstWait(backend));
    RTTESTI_CHECK(!fSuccess);
    const QByteArray after = tstRead(strRoot + "/calls.jsonl").mid(before.size());
    RTTESTI_CHECK(!after.contains("kill"));
    RTTESTI_CHECK(backend.devices().first().m_strSerial.isEmpty());
    tstControl(strRoot, "wrongIdentity", false);
    backend.refresh();
    RTTESTI_CHECK(tstWait(backend));
    backend.stopPhone(strId);
    RTTESTI_CHECK(tstWait(backend));
    RTTESTI_CHECK(fSuccess);
    backend.refresh();
    RTTESTI_CHECK(tstWait(backend));
    RTTESTI_CHECK(backend.devices().first().m_strSerial.isEmpty());
    RTTESTI_CHECK(!backend.devices().first().m_fBooted);

    RTTestISub("Failures, output limits and catalog containment");
    tstControl(strRoot, "adbFails", true);
    backend.refresh();
    RTTESTI_CHECK(tstWait(backend));
    RTTESTI_CHECK(!fSuccess);
    tstControl(strRoot, "adbFails", false);
    tstControl(strRoot, "createFails", true);
    backend.createPhone(options);
    RTTESTI_CHECK(tstWait(backend));
    RTTESTI_CHECK(!fSuccess);
    RTTESTI_CHECK(backend.devices().size() == 1);
    tstControl(strRoot, "createFails", false);
    tstControl(strRoot, "flood", true);
    backend.checkAcceleration();
    RTTESTI_CHECK(tstWait(backend));
    RTTESTI_CHECK(!fSuccess);
    RTTESTI_CHECK(strMessage.size() < 66560);
    tstControl(strRoot, "flood", false);
    tstControl(strRoot, "delay", 20000);
    backend.checkAcceleration();
    RTTESTI_CHECK(tstWait(backend, 20000));
    RTTESTI_CHECK(!fSuccess);
    RTTESTI_CHECK(strMessage.contains("time/output limit"));
    tstControl(strRoot, "delay", 0);
    RTTESTI_CHECK(tstWrite(strRoot + "/data/avd/" + strId + ".ini", ("path=" + strRoot + '\n').toUtf8()));
    UIPhoneBackend escaped(0, strRoot + "/data");
    RTTESTI_CHECK(escaped.devices().isEmpty());
    /* Every mutating ADB operation must use a verified emulator serial. */
    foreach (const QByteArray &line, tstRead(strRoot + "/calls.jsonl").split('\n'))
    {
        const QJsonObject call = QJsonDocument::fromJson(line).object();
        if (call.value("tool") != "adb")
            continue;
        const QJsonArray args = call.value("args").toArray();
        if (args.size() > 1)
        {
            RTTESTI_CHECK(args.at(0).toString() == "-s");
            RTTESTI_CHECK(args.at(1).toString().startsWith("emulator-"));
            RTTESTI_CHECK(!args.contains(QJsonValue("physical-phone")));
        }
    }

    RTTestISub("Varied profiles preserve independent AVDs");
    UIPhoneBackend fleet(0, strRoot + "/fleet-data");
    RTTESTI_CHECK(fleet.configure(strSdk, strJava, strError));
    options.m_fVaryProfile = true;
    const QList<UIPhoneProfile> profiles = UIPhoneBackend::profiles();
    for (qsizetype i = 0; i < profiles.size(); ++i)
    {
        fleet.createPhone(options);
        RTTESTI_CHECK(tstWait(fleet));
    }
    UIPhoneBackend reopened(0, fleet.dataRoot());
    RTTESTI_CHECK(reopened.devices().size() == profiles.size());
    for (const UIPhoneProfile &profile : profiles)
    {
        int cMatches = 0;
        for (const UIPhoneDevice &device : reopened.devices())
            if (device.m_strProfile == profile.m_pszSdkDevice)
            {
                ++cMatches;
                const QByteArray settings = tstRead(fleet.dataRoot() + "/avd/" + device.m_strId + ".avd/config.ini");
                RTTESTI_CHECK(settings.contains("hw.lcd.width=" + QByteArray::number(profile.m_iWidth) + '\n'));
                RTTESTI_CHECK(settings.contains("hw.lcd.height=" + QByteArray::number(profile.m_iHeight) + '\n'));
                RTTESTI_CHECK(settings.contains("hw.lcd.density=" + QByteArray::number(profile.m_iDensity) + '\n'));
            }
        RTTESTI_CHECK(cMatches == 1);
    }
}

/** Translator used to verify live changes without shipping a test catalog. */
class QITestPhoneTranslator : public QTranslator
{
public:
    virtual bool isEmpty() const RT_OVERRIDE { return false; }
    virtual QString translate(const char *pszContext, const char *pszSource,
                              const char *, int) const RT_OVERRIDE
    {
        return qstrcmp(pszContext, "UIPhoneManager") == 0 ? "T: " + QString::fromUtf8(pszSource) : QString();
    }
};

/** Checks phone setup, creation and navigation in an embedded stack. */
static void tstPage(const QString &strRoot, const QString &strSdk, const QString &strJava)
{
    const QString strSnapshots = qEnvironmentVariable("VBOX_GUI_TEST_SNAPSHOT_DIR");
    RTTestISub("Embedded page first use and translation");
    QStackedWidget stack;
    QWidget *pOtherPage = new QWidget(&stack);
    stack.addWidget(pOtherPage);
    UIPhoneManager page(&stack, strRoot + "/empty-page-data");
    stack.addWidget(&page);
    stack.resize(760, 640);
    stack.show();
    stack.setCurrentWidget(&page);
    RTTESTI_CHECK(!page.isWindow() && page.window() == &stack);
    QPushButton *pCreate = page.findChild<QPushButton*>("phoneCreate");
    QAction *pStart = page.findChild<QAction*>("phoneStart");
    QLineEdit *pSdk = page.findChild<QLineEdit*>("phoneSdkPath");
    QDialog *pCreateDialog = page.findChild<QDialog*>("phoneCreateDialog");
    QDialog *pSdkDialog = page.findChild<QDialog*>("phoneSdkPanel");
    const QList<QAction*> actions = page.toolbarActions();
    RTTESTI_CHECK_RETV(pCreate && pStart && pSdk && pCreateDialog && pSdkDialog && actions.size() == 8);
    QAction *pNewAction = actions.at(0);
    QAction *pSdkAction = actions.at(1);
    RTTESTI_CHECK(actions.contains(pStart) && !pStart->icon().pixmap(32, 32).isNull());
    RTTESTI_CHECK(!page.findChild<QLabel*>("phoneHeading") && !page.findChild<QPushButton*>("phoneStart"));
    RTTESTI_CHECK(!pCreate->isEnabled() && !pStart->isEnabled());
    RTTESTI_CHECK(!pSdk->accessibleName().isEmpty());
    QPushButton *pSdkDone = page.findChild<QPushButton*>("phoneSdkDone");
    QPushButton *pCancel = page.findChild<QPushButton*>("phoneCreateCancel");
    RTTESTI_CHECK_RETV(pSdkDone && pCancel && pSdkDialog->isHidden());
    QApplication::processEvents();
    pNewAction->trigger();
    QApplication::processEvents();
    RTTESTI_CHECK(pCreateDialog->isWindow() && pCreateDialog->windowModality() == Qt::WindowModal);
    RTTESTI_CHECK(pCreate->isVisible() && page.isVisible());
    QITestPhoneTranslator translator;
    qApp->installTranslator(&translator);
    QApplication::processEvents();
    RTTESTI_CHECK(page.accessibleName().startsWith("T: "));
    RTTESTI_CHECK(pCreate->text().startsWith("T: "));
    RTTESTI_CHECK(pNewAction->text().startsWith("T: ") && pSdkAction->text().startsWith("T: "));
    RTTESTI_CHECK(pCreateDialog->isVisible());
    qApp->removeTranslator(&translator);
    QApplication::processEvents();
    pCancel->click();
    RTTESTI_CHECK(pCreateDialog->isHidden());
    RTTestISub("Page SDK application and phone creation");
    QLineEdit *pJava = page.findChild<QLineEdit*>("phoneJavaHome");
    QLineEdit *pName = page.findChild<QLineEdit*>("phoneName");
    QPushButton *pApply = page.findChild<QPushButton*>("phoneApplySdk");
    QComboBox *pImage = page.findChild<QComboBox*>("phoneImage");
    QListWidget *pList = page.findChild<QListWidget*>("phoneList");
    UIPhoneBackend *pBackend = page.findChild<UIPhoneBackend*>();
    RTTESTI_CHECK_RETV(pJava && pName && pApply && pImage && pList && pBackend);
    pSdk->setText(strSdk);
    pJava->setText(strJava);
    {
        const QRect frame = stack.frameGeometry();
        pSdkAction->trigger();
        QApplication::processEvents();
        RTTESTI_CHECK(pSdkDialog->isVisible() && pSdkDialog->windowModality() == Qt::WindowModal);
        RTTESTI_CHECK(stack.frameGeometry() == frame);
        foreach (QWidget *pControl, QList<QWidget*>() << pJava << pName << pImage << pCreate << pApply)
            RTTESTI_CHECK(pControl->height() == pSdk->height());
        QPushButton *pBrowseSdk = page.findChild<QPushButton*>("phoneBrowseSdk");
        QPushButton *pBrowseJava = page.findChild<QPushButton*>("phoneBrowseJava");
        RTTESTI_CHECK(pBrowseSdk && pBrowseJava);
        if (pBrowseSdk && pBrowseJava)
        {
            RTTESTI_CHECK(pBrowseSdk->size() == pBrowseJava->size());
            RTTESTI_CHECK(pBrowseSdk->height() == pSdk->height());
        }
        RTTESTI_CHECK(pSdk->mapTo(pSdkDialog, QPoint()).x() == pJava->mapTo(pSdkDialog, QPoint()).x());
        RTTESTI_CHECK(pSdk->width() == pJava->width());
        RTTESTI_CHECK(pApply->size() == page.findChild<QPushButton*>("phoneCheckAcceleration")->size());
        RTTESTI_CHECK(pApply->size() == page.findChild<QPushButton*>("phoneSetupHelp")->size());
        if (!strSnapshots.isEmpty())
        {
            RTTESTI_CHECK(QDir().mkpath(strSnapshots));
            RTTESTI_CHECK(pSdkDialog->grab().save(strSnapshots + "/phone-sdk.png"));
        }
    }
    pApply->click();
    RTTESTI_CHECK(tstWait(*pBackend));
    RTTESTI_CHECK(pSdkDialog->isVisible());
    RTTESTI_CHECK(page.findChild<QPushButton*>("phoneCheckAcceleration")->isEnabled());
    pSdkDone->click();
    RTTESTI_CHECK(pSdkDialog->isHidden());
    pNewAction->trigger();
    QApplication::processEvents();
    RTTESTI_CHECK(pCreateDialog->isVisible() && !pStart->isEnabled());
    RTTESTI_CHECK(pImage->count() == 1);
    RTTESTI_CHECK(pImage->width() >= pImage->fontMetrics().horizontalAdvance(pImage->currentText()));
    pName->setText("UI test phone");
    RTTESTI_CHECK(pCreate->isEnabled());
    if (!strSnapshots.isEmpty())
        RTTESTI_CHECK(pCreateDialog->grab().save(strSnapshots + "/phone-1.png"));
    pCreate->click();
    RTTESTI_CHECK(tstWait(*pBackend));
    RTTESTI_CHECK(pList->count() == 1 && pCreateDialog->isHidden());
    RTTESTI_CHECK(pList->count() && !pList->item(0)->icon().pixmap(24, 24).isNull());
    RTTESTI_CHECK(pName->height() >= pName->sizeHint().height());
    RTTESTI_CHECK(pImage->height() >= pImage->sizeHint().height());
    RTTESTI_CHECK(pStart->isEnabled());
    QPlainTextEdit *pResult = page.findChild<QPlainTextEdit*>("phoneResult");
    RTTESTI_CHECK(pResult && pResult->isHidden());
    RTTESTI_CHECK(!page.findChild<QAction*>("phoneInstallApk")->isEnabled());
    QLabel *pSelectedName = page.findChild<QLabel*>("phoneSelectedName");
    QLabel *pSelectedMemory = page.findChild<QLabel*>("phoneSelectedMemory");
    QSpinBox *pMemory = page.findChild<QSpinBox*>("phoneMemory");
    RTTESTI_CHECK_RETV(pSelectedName && pSelectedMemory && pMemory);
    RTTESTI_CHECK(pSelectedName->text() == pName->text() && pSelectedMemory->text() == "2048 MB");
    pNewAction->trigger();
    RTTESTI_CHECK(pCreateDialog->isVisible() && pSelectedName->isVisible());
    pName->setText("Second UI phone");
    pMemory->setValue(3072);
    pCreate->click();
    RTTESTI_CHECK(tstWait(*pBackend));
    RTTESTI_CHECK(pList->count() == 2 && !pCreateDialog->isVisible());
    RTTESTI_CHECK(pSelectedName->text() == pName->text() && pSelectedMemory->text() == "3072 MB");
    const int iOtherRow = pList->currentRow() == 0 ? 1 : 0;
    pList->clearSelection();
    RTTESTI_CHECK(!pStart->isEnabled() && !pSelectedName->isVisible());
    pList->setCurrentRow(iOtherRow);
    RTTESTI_CHECK(pStart->isEnabled() && pSelectedName->isVisible());
    RTTESTI_CHECK(pSelectedName->text() == "UI test phone" && pSelectedMemory->text() == "2048 MB");
    if (!strSnapshots.isEmpty())
        RTTESTI_CHECK(page.grab().save(strSnapshots + "/phone-0.png"));
    RTTestISub("Phone catalog survives page switching");
    stack.setCurrentWidget(pOtherPage);
    RTTESTI_CHECK(!page.isVisible());
    stack.setCurrentWidget(&page);
    QApplication::processEvents();
    RTTESTI_CHECK(tstWait(*pBackend));
    RTTESTI_CHECK(page.isVisible());
    RTTESTI_CHECK(!pCreateDialog->isVisible());
    RTTESTI_CHECK(pList->count() == 2 && pStart->isEnabled() && pSelectedName->text() == "UI test phone");
}

int main(int argc, char **argv)
{
    const QString strTool = QFileInfo(QString::fromLocal8Bit(argv[0])).completeBaseName();
    if (qEnvironmentVariableIsSet("NAVEBOX_PHONE_FAKE_ROOT") && (strTool == "java" || strTool == "adb" || strTool == "emulator"))
    {
        QCoreApplication app(argc, argv);
        return tstFakeTool(strTool);
    }
    RTTEST hTest;
    const int rc = RTTestInitAndCreate("tstNaveboxPhone", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    const QString strStyle = qEnvironmentVariable("VBOX_GUI_TEST_STYLE", "Fusion");
    RTTESTI_CHECK(app.setStyle(strStyle));
    const QString strFont = qEnvironmentVariable("VBOX_GUI_TEST_FONT");
    if (!strFont.isEmpty())
    {
        const int iFont = QFontDatabase::addApplicationFont(strFont);
        RTTESTI_CHECK(iFont >= 0);
        if (iFont >= 0)
            app.setFont(QFont(QFontDatabase::applicationFontFamilies(iFont).value(0), 9));
    }
    if (!qEnvironmentVariableIsSet("VBOX_GUI_TEST_NO_APP_STYLE"))
        app.setStyleSheet(UIAppearance::styleSheet(app.palette()));
    UIDesktopWidgetWatchdog::create();
    QTemporaryDir directory;
    RTTESTI_CHECK(directory.isValid());
    if (directory.isValid())
    {
        QString strSdk;
        QString strJava;
        qputenv("NAVEBOX_PHONE_FAKE_ROOT", directory.path().toUtf8());
        const bool fPrepared = tstPrepareSdk(directory.path(), strSdk, strJava);
        RTTESTI_CHECK(fPrepared);
        if (fPrepared)
            tstBackend(directory.path(), strSdk, strJava);
        if (fPrepared)
            tstPage(directory.path(), strSdk, strJava);
        qunsetenv("NAVEBOX_PHONE_FAKE_ROOT");
    }
    UIDesktopWidgetWatchdog::destroy();
    return RTTestSummaryAndDestroy(hTest);
}
