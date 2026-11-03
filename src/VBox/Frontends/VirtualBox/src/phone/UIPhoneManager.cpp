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

/* Qt includes: */
#include <QAction>
#include <QComboBox>
#include <QDesktopServices>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QThread>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIPhoneBackend.h"
#include "UIPhoneManager.h"

/** @page pg_navebox_phone_manager Navebox Android Phone Manager
 *
 * The File / Android Phones action opens an SDK-backed phone catalog. Phone
 * data is separate from VirtualBox VMs and from Android Studio's AVD catalog.
 * Closing this dialog or Navebox leaves detached emulator windows running.
 */

UIPhoneManager::UIPhoneManager(QWidget *pParent, const QString &strDataRoot)
    : QIDialog(pParent)
    , m_pBackend(new UIPhoneBackend(this, strDataRoot))
    , m_iSdkCollapsedHeight(0)
{
    setObjectName("androidPhoneManager");
    setMinimumWidth(680);
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(22, 18, 22, 18);
    pLayout->setSpacing(14);
    prepareHeader(pLayout);
    prepareSdk(pLayout);
    m_pTabs = new QTabWidget(this);
    m_pTabs->setObjectName("phoneTabs");
    m_pTabs->setDocumentMode(true);
    pLayout->addWidget(m_pTabs, 1);
    preparePhones();
    prepareCreation();
    prepareFooter(pLayout);
    prepareAppearance();

    /* Return in a path or name field must not start a phone. */
    foreach (QPushButton *pButton, findChildren<QPushButton*>())
        pButton->setAutoDefault(false);
    prepareConnections();
    retranslateUi();
    updateImages();
}

void UIPhoneManager::prepareAppearance()
{
    /* Scope the complete control styling to this dialog so secondary buttons
     * use the same frame and spacing as primary actions on native Windows. */
    setStyleSheet(QStringLiteral(
        "QFrame[phoneCard=\"true\"] { background: palette(base); border: 1px solid palette(midlight); border-radius: 10px; }"
        "QLabel#phoneHeading { font-size: 23px; font-weight: 700; }"
        "QLabel[phoneSection=\"true\"] { font-size: 15px; font-weight: 600; }"
        "QPushButton, QToolButton#phoneMore { background: palette(base); color: palette(button-text);"
        " border: 1px solid palette(midlight); border-radius: 6px; padding: 4px 12px; }"
        "QPushButton:hover, QToolButton#phoneMore:hover { background: palette(alternate-base); border-color: palette(mid); }"
        "QPushButton:pressed, QToolButton#phoneMore:pressed { background: palette(button); }"
        "QPushButton:focus, QToolButton#phoneMore:focus { border-color: palette(highlight); }"
        "QPushButton:disabled, QToolButton#phoneMore:disabled { background: palette(window); color: palette(mid); }"
        "QPushButton#phoneSdkSettings:checked { background: palette(alternate-base); border-color: palette(highlight); }"
        "QLineEdit, QSpinBox, QComboBox { background: palette(base); color: palette(text);"
        " border: 1px solid palette(midlight); border-radius: 6px; padding: 4px 10px; }"
        "QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: palette(highlight); }"
        "QSpinBox QLineEdit { background: transparent; border: none; padding: 0; }"
        "QComboBox::drop-down { border: none; background: transparent; width: 22px; }"
        "QSpinBox::up-button, QSpinBox::down-button { border: none; background: transparent; width: 20px; }"
        "QTabWidget#phoneTabs::pane { border: none; }"
        "QTabBar::tab { background: transparent; padding: 9px 15px; border-bottom: 2px solid transparent; }"
        "QTabBar::tab:selected { border-bottom-color: palette(highlight); font-weight: 600; }"
        "QTabBar::tab:focus { background: palette(alternate-base); }"
        "QFrame#phonePreviewDevice { border: 2px solid palette(highlight); border-radius: 18px; }"
        "QLabel#phonePreviewScreen { color: palette(highlight); font-weight: 600; }"
        "QPushButton[phonePrimary=\"true\"] { background: palette(highlight); color: palette(highlighted-text);"
        " border: 1px solid palette(highlight); font-weight: 600; }"
        "QPushButton[phonePrimary=\"true\"]:focus { border-color: palette(window-text); }"
        "QPushButton[phonePrimary=\"true\"]:disabled { background: palette(button); color: palette(button-text);"
        " border-color: palette(midlight); }"
        "QPushButton[phonePrimary=\"true\"]:hover { border-color: palette(window-text); }"
        "QTableWidget#phoneTable { border: 1px solid palette(midlight); border-radius: 8px; }"
        "QTableWidget#phoneTable::item { padding: 6px 10px; }"
        "QTableWidget#phoneTable QHeaderView::section { background: palette(alternate-base);"
        " padding: 7px 10px; border: none; font-weight: 600; }"
        "QToolButton#phoneMore { padding-right: 22px; }"
        "QToolButton#phoneMore::menu-indicator { subcontrol-origin: padding; subcontrol-position: right center; right: 8px; }"));
}

void UIPhoneManager::prepareHeader(QVBoxLayout *pLayout)
{
    QHBoxLayout *pHeader = new QHBoxLayout;
    pHeader->setSpacing(16);
    QVBoxLayout *pTitle = new QVBoxLayout;
    pTitle->setSpacing(4);
    m_pHeading = new QLabel(this);
    m_pHeading->setObjectName("phoneHeading");
    pTitle->addWidget(m_pHeading);
    m_pRuntimeState = new QLabel(this);
    m_pRuntimeState->setObjectName("phoneRuntimeState");
    pTitle->addWidget(m_pRuntimeState);
    pHeader->addLayout(pTitle, 1);
    m_pSdkSettings = new QPushButton(this);
    m_pSdkSettings->setObjectName("phoneSdkSettings");
    m_pSdkSettings->setCheckable(true);
    pHeader->addWidget(m_pSdkSettings, 0, Qt::AlignVCenter);
    pLayout->addLayout(pHeader);
}

void UIPhoneManager::prepareSdk(QVBoxLayout *pLayout)
{
    m_pSdkGroup = new QFrame(this);
    m_pSdkGroup->setObjectName("phoneSdkPanel");
    m_pSdkGroup->setProperty("phoneCard", true);
    m_pSdkGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    pLayout->addWidget(m_pSdkGroup);
    QVBoxLayout *pSdkLayout = new QVBoxLayout(m_pSdkGroup);
    pSdkLayout->setContentsMargins(16, 14, 16, 16);
    pSdkLayout->setSpacing(14);
    m_pSdkTitle = new QLabel(m_pSdkGroup);
    m_pSdkTitle->setProperty("phoneSection", true);
    pSdkLayout->addWidget(m_pSdkTitle);
    m_pSdkForm = new QFormLayout;
    pSdkLayout->addLayout(m_pSdkForm);
    m_pSdkForm->setContentsMargins(0, 0, 0, 0);
    m_pSdkForm->setHorizontalSpacing(12);
    m_pSdkForm->setVerticalSpacing(10);
    m_pSdkForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_pSdkForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    m_pSdkPath = new QLineEdit(m_pBackend->sdkRoot().isEmpty() ? UIPhoneBackend::suggestedSdkRoot()
                                                            : m_pBackend->sdkRoot(), this);
    m_pSdkPath->setObjectName("phoneSdkPath");
    m_pJavaHome = new QLineEdit(m_pBackend->javaHome().isEmpty() ? UIPhoneBackend::suggestedJavaHome()
                                                             : m_pBackend->javaHome(), this);
    m_pJavaHome->setObjectName("phoneJavaHome");
    m_pBrowseSdk = new QPushButton(this);
    m_pBrowseSdk->setObjectName("phoneBrowseSdk");
    m_pBrowseJava = new QPushButton(this);
    m_pBrowseJava->setObjectName("phoneBrowseJava");
    QHBoxLayout *pSdkRow = new QHBoxLayout;
    pSdkRow->setSpacing(8);
    pSdkRow->addWidget(m_pSdkPath, 1);
    pSdkRow->addWidget(m_pBrowseSdk);
    m_pSdkForm->addRow(new QLabel(this), pSdkRow);
    QHBoxLayout *pJavaRow = new QHBoxLayout;
    pJavaRow->setSpacing(8);
    pJavaRow->addWidget(m_pJavaHome, 1);
    pJavaRow->addWidget(m_pBrowseJava);
    m_pSdkForm->addRow(new QLabel(this), pJavaRow);
    m_pApply = new QPushButton(this);
    m_pApply->setObjectName("phoneApplySdk");
    m_pCheck = new QPushButton(this);
    m_pCheck->setObjectName("phoneCheckAcceleration");
    m_pHelp = new QPushButton(this);
    m_pHelp->setObjectName("phoneSetupHelp");
    QHBoxLayout *pSetupActions = new QHBoxLayout;
    pSetupActions->setSpacing(8);
    pSetupActions->addWidget(m_pApply);
    pSetupActions->addWidget(m_pCheck);
    pSetupActions->addStretch();
    pSetupActions->addWidget(m_pHelp);
    m_pSdkForm->addRow(pSetupActions);
    m_pSdkGroup->hide();
}

void UIPhoneManager::preparePhones()
{
    QWidget *pPhones = new QWidget(this);
    QVBoxLayout *pPhonesLayout = new QVBoxLayout(pPhones);
    pPhonesLayout->setContentsMargins(0, 12, 0, 0);
    pPhonesLayout->setSpacing(12);
    QFrame *pEmptyCard = new QFrame(pPhones);
    pEmptyCard->setProperty("phoneCard", true);
    m_pEmptyCard = pEmptyCard;
    QVBoxLayout *pEmptyLayout = new QVBoxLayout(pEmptyCard);
    pEmptyLayout->setContentsMargins(24, 22, 24, 22);
    pEmptyLayout->setSpacing(10);
    pEmptyLayout->addStretch();
    QFrame *pSymbol = new QFrame(pEmptyCard);
    pSymbol->setObjectName("phonePreviewDevice");
    pSymbol->setFixedSize(34, 54);
    pEmptyLayout->addWidget(pSymbol, 0, Qt::AlignHCenter);
    m_pEmptyTitle = new QLabel(pEmptyCard);
    m_pEmptyTitle->setProperty("phoneSection", true);
    m_pEmptyTitle->setAlignment(Qt::AlignCenter);
    pEmptyLayout->addWidget(m_pEmptyTitle);
    m_pEmpty = new QLabel(pEmptyCard);
    m_pEmpty->setAlignment(Qt::AlignCenter);
    m_pEmpty->setWordWrap(true);
    pEmptyLayout->addWidget(m_pEmpty);
    m_pEmptyAction = new QPushButton(pEmptyCard);
    m_pEmptyAction->setProperty("phonePrimary", true);
    pEmptyLayout->addWidget(m_pEmptyAction, 0, Qt::AlignHCenter);
    pEmptyLayout->addStretch();
    pPhonesLayout->addWidget(pEmptyCard, 1);
    m_pTable = new QTableWidget(0, 2, pPhones);
    m_pTable->setObjectName("phoneTable");
    m_pTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pTable->verticalHeader()->hide();
    m_pTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_pTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_pTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_pTable->setShowGrid(false);
    m_pTable->setAlternatingRowColors(true);
    m_pTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    pPhonesLayout->addWidget(m_pTable, 1);
    m_pPhoneActions = new QWidget(pPhones);
    m_pRefresh = new QPushButton(m_pPhoneActions);
    m_pStart = new QPushButton(m_pPhoneActions);
    m_pStart->setObjectName("phoneStart");
    m_pStart->setProperty("phonePrimary", true);
    m_pStop = new QPushButton(m_pPhoneActions);
    m_pInstall = new QPushButton(m_pPhoneActions);
    m_pInstall->setObjectName("phoneInstallApk");
    QHBoxLayout *pRunActions = new QHBoxLayout(m_pPhoneActions);
    pRunActions->setSpacing(8);
    pRunActions->setContentsMargins(0, 0, 0, 0);
    for (QPushButton *pButton : {m_pStart, m_pStop, m_pInstall})
        pRunActions->addWidget(pButton);
    pRunActions->addStretch();
    pRunActions->addWidget(m_pRefresh);
    m_pMore = new QToolButton(m_pPhoneActions);
    m_pMore->setObjectName("phoneMore");
    m_pMore->setPopupMode(QToolButton::InstantPopup);
    QMenu *pMore = new QMenu(m_pMore);
    m_pColdBoot = pMore->addAction(QString());
    m_pLog = pMore->addAction(QString());
    pMore->addSeparator();
    m_pBack = pMore->addAction(QString());
    m_pHome = pMore->addAction(QString());
    m_pRecent = pMore->addAction(QString());
    m_pMore->setMenu(pMore);
    pRunActions->addWidget(m_pMore);
    pPhonesLayout->addWidget(m_pPhoneActions);
    m_pTabs->addTab(pPhones, QString());
}

void UIPhoneManager::prepareCreation()
{
    QWidget *pCreate = new QWidget(this);
    QVBoxLayout *pCreateLayout = new QVBoxLayout(pCreate);
    pCreateLayout->setContentsMargins(0, 12, 0, 0);
    pCreateLayout->setSpacing(12);
    QHBoxLayout *pCards = new QHBoxLayout;
    pCards->setSpacing(14);
    pCreateLayout->addLayout(pCards, 1);
    QFrame *pFormCard = new QFrame(pCreate);
    pFormCard->setProperty("phoneCard", true);
    QVBoxLayout *pFormCardLayout = new QVBoxLayout(pFormCard);
    pFormCardLayout->setContentsMargins(16, 14, 16, 16);
    pFormCardLayout->setSpacing(12);
    m_pCreateTitle = new QLabel(pFormCard);
    m_pCreateTitle->setProperty("phoneSection", true);
    pFormCardLayout->addWidget(m_pCreateTitle);
    m_pCreateForm = new QFormLayout;
    m_pCreateForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_pCreateForm->setSpacing(9);
    m_pCreateForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    pFormCardLayout->addLayout(m_pCreateForm);
    pFormCardLayout->addStretch();
    pCards->addWidget(pFormCard, 1);
    m_pName = new QLineEdit(pFormCard);
    m_pName->setObjectName("phoneName");
    m_pName->setMaxLength(64);
    m_pImage = new QComboBox(pFormCard);
    m_pImage->setObjectName("phoneImage");
    m_pImage->setMinimumContentsLength(28);
    m_pImage->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_pImage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_pMemory = new QSpinBox(pFormCard);
    m_pMemory->setObjectName("phoneMemory");
    m_pMemory->setRange(1024, 16384);
    m_pMemory->setSingleStep(256);
    m_pMemory->setValue(2048);
    m_pCpus = new QSpinBox(pFormCard);
    m_pCpus->setObjectName("phoneCpus");
    m_pCpus->setRange(1, qMax(1, qMin(8, QThread::idealThreadCount())));
    m_pCpus->setValue(qMin(2, m_pCpus->maximum()));
    m_pStorage = new QComboBox(pFormCard);
    for (int iSize : {8, 16, 32, 64})
        m_pStorage->addItem(QString::number(iSize) + " GB", iSize);
    m_pDisplay = new QComboBox(pFormCard);
    m_pDisplay->addItem("720 x 1280 (320 dpi)", QSize(720, 1280));
    m_pDisplay->addItem("1080 x 1920 (420 dpi)", QSize(1080, 1920));
    foreach (QWidget *pField, QList<QWidget*>() << m_pName << m_pImage << m_pMemory << m_pCpus << m_pStorage << m_pDisplay)
    {
        pField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QLabel *pLabel = new QLabel(pFormCard);
        pLabel->setBuddy(pField);
        m_pCreateForm->addRow(pLabel, pField);
    }
    QFrame *pPreview = new QFrame(pCreate);
    pPreview->setProperty("phoneCard", true);
    QVBoxLayout *pPreviewLayout = new QVBoxLayout(pPreview);
    pPreviewLayout->setContentsMargins(16, 14, 16, 16);
    pPreviewLayout->setSpacing(8);
    m_pPreviewTitle = new QLabel(pPreview);
    m_pPreviewTitle->setProperty("phoneSection", true);
    m_pPreviewTitle->setAlignment(Qt::AlignCenter);
    pPreviewLayout->addWidget(m_pPreviewTitle);
    pPreviewLayout->addStretch();
    QFrame *pDevice = new QFrame(pPreview);
    pDevice->setObjectName("phonePreviewDevice");
    pDevice->setFixedSize(90, 158);
    QVBoxLayout *pScreenLayout = new QVBoxLayout(pDevice);
    m_pPreviewScreen = new QLabel(pDevice);
    m_pPreviewScreen->setObjectName("phonePreviewScreen");
    m_pPreviewScreen->setAlignment(Qt::AlignCenter);
    pScreenLayout->addWidget(m_pPreviewScreen);
    pPreviewLayout->addWidget(pDevice, 0, Qt::AlignHCenter);
    m_pPreviewDisplay = new QLabel(pPreview);
    m_pPreviewDisplay->setAlignment(Qt::AlignCenter);
    pPreviewLayout->addWidget(m_pPreviewDisplay);
    m_pPreviewMemory = new QLabel(pPreview);
    m_pPreviewMemory->setAlignment(Qt::AlignCenter);
    pPreviewLayout->addWidget(m_pPreviewMemory);
    pPreviewLayout->addStretch();
    pCards->addWidget(pPreview);
    QHBoxLayout *pCreateActions = new QHBoxLayout;
    m_pCreateHelp = new QLabel(pCreate);
    m_pCreateHelp->setWordWrap(true);
    pCreateActions->addWidget(m_pCreateHelp, 1);
    m_pCreate = new QPushButton(pCreate);
    m_pCreate->setObjectName("phoneCreate");
    m_pCreate->setProperty("phonePrimary", true);
    pCreateActions->addWidget(m_pCreate);
    pCreateLayout->addLayout(pCreateActions);
    m_pTabs->addTab(pCreate, QString());
}

void UIPhoneManager::prepareFooter(QVBoxLayout *pLayout)
{
    m_pResult = new QPlainTextEdit(this);
    m_pResult->setObjectName("phoneResult");
    m_pResult->setReadOnly(true);
    m_pResult->setMaximumBlockCount(200);
    m_pResult->setMaximumHeight(110);
    pLayout->addWidget(m_pResult);
    m_pResult->hide();
    QHBoxLayout *pFooter = new QHBoxLayout;
    pFooter->setSpacing(8);
    m_pNotice = new QLabel(this);
    m_pNotice->setObjectName("phoneNotice");
    m_pNotice->setTextFormat(Qt::PlainText);
    m_pNotice->setWordWrap(true);
    pFooter->addWidget(m_pNotice, 1);
    m_pDetails = new QPushButton(this);
    m_pDetails->setObjectName("phoneDetails");
    m_pDetails->setCheckable(true);
    pFooter->addWidget(m_pDetails);
    m_pDetails->hide();
    m_pClose = new QPushButton(this);
    m_pClose->setObjectName("phoneClose");
    pFooter->addWidget(m_pClose);
    pLayout->addLayout(pFooter);
}

void UIPhoneManager::prepareConnections()
{
    connect(m_pSdkSettings, &QPushButton::toggled, this, [this](bool fExpanded)
    {
        if (fExpanded)
            m_iSdkCollapsedHeight = height();
        m_pSdkSettings->setText(fExpanded ? tr("Done") : tr("SDK Settings"));
        updateControlGeometry();
        m_pSdkGroup->setVisible(fExpanded);
        m_pTabs->setVisible(!fExpanded);
        layout()->activate();
        resize(width(), fExpanded ? minimumSizeHint().height()
                                 : qMax(m_iSdkCollapsedHeight, minimumSizeHint().height()));
    });
    connect(m_pDetails, &QPushButton::toggled, m_pResult, &QWidget::setVisible);
    connect(m_pEmptyAction, &QPushButton::clicked, this, [this]()
    {
        if (m_pImage->count() > 0)
        {
            m_pTabs->setCurrentIndex(1);
            m_pName->setFocus();
        }
        else
        {
            m_pSdkSettings->setChecked(true);
            m_pSdkPath->setFocus();
        }
    });
    connect(m_pBrowseSdk, &QPushButton::clicked, this, [this]()
    {
        const QString strPath = QFileDialog::getExistingDirectory(this, tr("Choose Android SDK"), m_pSdkPath->text());
        if (!strPath.isEmpty())
            m_pSdkPath->setText(strPath);
    });
    connect(m_pBrowseJava, &QPushButton::clicked, this, [this]()
    {
        const QString strPath = QFileDialog::getExistingDirectory(this, tr("Choose Java Home"), m_pJavaHome->text());
        if (!strPath.isEmpty())
            m_pJavaHome->setText(strPath);
    });
    connect(m_pApply, &QPushButton::clicked, this, [this]() { applySdk(); });
    connect(m_pCheck, &QPushButton::clicked, m_pBackend, &UIPhoneBackend::checkAcceleration);
    connect(m_pHelp, &QPushButton::clicked, this, []()
    {
        QDesktopServices::openUrl(QUrl("https://developer.android.com/tools/sdkmanager"));
    });
    connect(m_pRefresh, &QPushButton::clicked, this, [this]() { refresh(); });
    connect(m_pStart, &QPushButton::clicked, this, [this]() { m_pBackend->startPhone(selectedId()); });
    connect(m_pColdBoot, &QAction::triggered, this, [this]() { m_pBackend->startPhone(selectedId(), true); });
    connect(m_pStop, &QPushButton::clicked, this, [this]() { m_pBackend->stopPhone(selectedId()); });
    connect(m_pInstall, &QPushButton::clicked, this, [this]()
    {
        const QString strId = selectedId();
        const QString strPath = QFileDialog::getOpenFileName(this, tr("Install Android APK"), QString(), tr("Android packages (*.apk)"));
        if (!strPath.isEmpty())
            m_pBackend->installApk(strId, strPath);
    });
    connect(m_pBack, &QAction::triggered, this, [this]() { m_pBackend->sendKey(selectedId(), 4); });
    connect(m_pHome, &QAction::triggered, this, [this]() { m_pBackend->sendKey(selectedId(), 3); });
    connect(m_pRecent, &QAction::triggered, this, [this]() { m_pBackend->sendKey(selectedId(), 187); });
    connect(m_pLog, &QAction::triggered, this, [this]()
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_pBackend->dataRoot() + '/' + selectedId() + ".log"));
    });
    connect(m_pCreate, &QPushButton::clicked, this, [this]() { createPhone(); });
    connect(m_pClose, &QPushButton::clicked, this, &UIPhoneManager::hide);
    connect(m_pTable, &QTableWidget::itemSelectionChanged, this, [this]() { updateControls(); });
    connect(m_pName, &QLineEdit::textChanged, this, [this]() { updateControls(); });
    connect(m_pSdkPath, &QLineEdit::textChanged, this, [this]() { updateControls(); });
    connect(m_pJavaHome, &QLineEdit::textChanged, this, [this]() { updateControls(); });
    connect(m_pMemory, &QSpinBox::valueChanged, this, [this]() { updatePreview(); });
    connect(m_pCpus, &QSpinBox::valueChanged, this, [this]() { updatePreview(); });
    connect(m_pDisplay, &QComboBox::currentIndexChanged, this, [this]() { updatePreview(); });
    connect(m_pBackend, &UIPhoneBackend::sigBusyChanged, this, [this]() { updateControls(); });
    connect(m_pBackend, &UIPhoneBackend::sigDevicesChanged, this, [this]() { updateDevices(); });
    connect(m_pBackend, &UIPhoneBackend::sigOperationFinished, this, &UIPhoneManager::showResult);
    connect(m_pBackend, &UIPhoneBackend::sigPhoneCreated, this, [this](const QString &strId)
    {
        m_pTabs->setCurrentIndex(0);
        for (int iRow = 0; iRow < m_pTable->rowCount(); ++iRow)
            if (m_pTable->item(iRow, 0)->data(Qt::UserRole).toString() == strId)
            {
                m_pTable->selectRow(iRow);
                break;
            }
    });
    QTimer *pPoll = new QTimer(this);
    pPoll->setInterval(5000);
    connect(pPoll, &QTimer::timeout, this, [this]()
    {
        if (isVisible() && !m_pBackend->isBusy())
        {
            QString strError;
            if (m_pBackend->isConfigured(strError))
                m_pBackend->refresh();
        }
    });
    pPoll->start();
}

void UIPhoneManager::changeEvent(QEvent *pEvent)
{
    QIDialog::changeEvent(pEvent);
    if (pEvent->type() == QEvent::LanguageChange)
        retranslateUi();
    else if (pEvent->type() == QEvent::FontChange || pEvent->type() == QEvent::StyleChange)
        updateControlGeometry();
}

void UIPhoneManager::updateControlGeometry()
{
    QList<QWidget*> controls = QList<QWidget*>() << m_pSdkPath << m_pJavaHome << m_pName << m_pImage
                                                << m_pMemory << m_pCpus << m_pStorage << m_pDisplay << m_pMore;
    foreach (QPushButton *pButton, findChildren<QPushButton*>())
    {
        pButton->setMinimumWidth(qMax(88, pButton->fontMetrics().horizontalAdvance(pButton->text()) + 28));
        controls << pButton;
    }
    /* Give every field and action one shared height, including hidden pages.
     * Recalculate after font/style changes to preserve text at larger DPI. */
    int cHeight = qMax(34, fontMetrics().height() + 16);
    foreach (QWidget *pControl, controls)
    {
        pControl->ensurePolished();
        cHeight = qMax(cHeight, pControl->sizeHint().height());
    }
    foreach (QWidget *pControl, controls)
        pControl->setFixedHeight(cHeight);
    const int cBrowseWidth = qMax(m_pBrowseSdk->minimumWidth(), m_pBrowseJava->minimumWidth());
    m_pBrowseSdk->setFixedWidth(cBrowseWidth);
    m_pBrowseJava->setFixedWidth(cBrowseWidth);
    const int cSetupWidth = qMax(m_pCheck->minimumWidth(), qMax(m_pApply->minimumWidth(), m_pHelp->minimumWidth()));
    for (QPushButton *pButton : {m_pApply, m_pCheck, m_pHelp})
        pButton->setFixedWidth(cSetupWidth);
    const int cActionWidth = qMax(108, qMax(m_pSdkSettings->minimumWidth(),
                                         qMax(m_pCreate->minimumWidth(), qMax(m_pClose->minimumWidth(), m_pDetails->minimumWidth()))));
    for (QPushButton *pButton : {m_pSdkSettings, m_pCreate, m_pClose, m_pDetails})
        pButton->setFixedWidth(cActionWidth);
    m_pMore->setMinimumWidth(88);
    m_pMemory->setMinimumWidth(m_pMemory->fontMetrics().horizontalAdvance(QString::number(m_pMemory->maximum())
                                                                        + m_pMemory->suffix()) + 42);
    m_pCpus->setMinimumWidth(m_pCpus->fontMetrics().horizontalAdvance(QString::number(m_pCpus->maximum())) + 42);
}

void UIPhoneManager::retranslateUi()
{
    setWindowTitle(tr("Android Phones - Navebox"));
    m_pHeading->setText(tr("Android Phones"));
    m_pSdkSettings->setText(m_pSdkSettings->isChecked() ? tr("Done") : tr("SDK Settings"));
    m_pSdkTitle->setText(tr("Android SDK"));
    m_pSdkPath->setAccessibleName(tr("Android SDK directory"));
    m_pJavaHome->setAccessibleName(tr("Java home"));
    qobject_cast<QLabel*>(m_pSdkForm->itemAt(0, QFormLayout::LabelRole)->widget())->setText(tr("SDK directory:"));
    qobject_cast<QLabel*>(m_pSdkForm->itemAt(1, QFormLayout::LabelRole)->widget())->setText(tr("Java home:"));
    m_pBrowseSdk->setText(tr("Browse..."));
    m_pBrowseJava->setText(tr("Browse..."));
    m_pApply->setText(tr("Apply SDK"));
    m_pCheck->setText(tr("Check Acceleration"));
    m_pHelp->setText(tr("Setup Help"));
    m_pTabs->setTabText(0, tr("Phones"));
    m_pTabs->setTabText(1, tr("Create Phone"));
    m_pTable->setHorizontalHeaderLabels(QStringList() << tr("Phone") << tr("Status"));
    m_pTable->setAccessibleName(tr("Android phones"));
    m_pRefresh->setText(tr("Refresh"));
    m_pStart->setText(tr("Start"));
    m_pColdBoot->setText(tr("Cold Boot"));
    m_pColdBoot->setToolTip(tr("Start without loading a Quick Boot snapshot; user data is preserved."));
    m_pStop->setText(tr("Stop"));
    m_pInstall->setText(tr("Install APK..."));
    m_pMore->setText(tr("More"));
    m_pMore->setAccessibleName(tr("More phone actions"));
    m_pBack->setText(tr("Back"));
    m_pHome->setText(tr("Home"));
    m_pRecent->setText(tr("Recent Apps"));
    m_pLog->setText(tr("Open Log"));
    const QStringList labels = QStringList() << tr("Phone name:") << tr("Android image:") << tr("Memory:")
                                             << tr("CPUs:") << tr("Storage:") << tr("Display:");
    for (int i = 0; i < labels.size(); ++i)
        qobject_cast<QLabel*>(m_pCreateForm->itemAt(i, QFormLayout::LabelRole)->widget())->setText(labels.at(i));
    m_pName->setPlaceholderText(tr("My Android Phone"));
    m_pMemory->setSuffix(tr(" MB"));
    m_pCreateTitle->setText(tr("Phone configuration"));
    m_pPreviewTitle->setText(tr("Device preview"));
    m_pPreviewScreen->setText(tr("Android"));
    m_pCreate->setText(tr("Create Phone"));
    m_pResult->setAccessibleName(tr("Phone operation result"));
    m_pDetails->setText(tr("Details"));
    m_pClose->setText(tr("Close"));
    if (m_pResult->toPlainText().isEmpty())
        m_pNotice->setText(tr("Phones keep running when you close this window."));
    updateControlGeometry();
    updatePreview();
    updateDevices();
}

void UIPhoneManager::updatePreview()
{
    const QSize size = m_pDisplay->currentData().toSize();
    m_pPreviewDisplay->setText(tr("%1 x %2 pixels").arg(size.width()).arg(size.height()));
    const QString strCpus = m_pCpus->value() == 1 ? tr("1 CPU") : tr("%1 CPUs").arg(m_pCpus->value());
    m_pPreviewMemory->setText(tr("%1 MB RAM / %2").arg(m_pMemory->value()).arg(strCpus));
}

void UIPhoneManager::showResult(bool fSuccess, const QString &strMessage)
{
    const QString strText = strMessage.trimmed();
    m_pResult->setPlainText(strText);
    QString strSummary = strText.section('\n', 0, 0);
    const bool fDetails = !fSuccess || strText.contains('\n') || strSummary.size() > 135;
    if (strSummary.size() > 135)
        strSummary = strSummary.left(132) + "...";
    m_pNotice->setText(fSuccess ? strSummary : tr("Error: %1").arg(strSummary));
    m_pDetails->setChecked(false);
    m_pDetails->setVisible(fDetails);
}

void UIPhoneManager::applySdk()
{
    QString strError;
    if (!m_pBackend->configure(m_pSdkPath->text().trimmed(), m_pJavaHome->text().trimmed(), strError))
    {
        showResult(false, strError);
        return;
    }
    m_pSdkPath->setText(m_pBackend->sdkRoot());
    m_pJavaHome->setText(m_pBackend->javaHome());
    updateImages();
    showResult(true, m_pImage->count() ? tr("Android runtime ready.")
                                      : tr("No compatible images. Install a phone image, then apply the SDK again."));
    refresh();
}

void UIPhoneManager::updateImages()
{
    const QString strSelected = m_pImage->currentData().toString();
    m_pImage->clear();
    foreach (const UIPhoneImage &image, m_pBackend->images())
        m_pImage->addItem(image.m_strLabel, image.m_strPackage);
    const int iIndex = m_pImage->findData(strSelected);
    if (iIndex >= 0)
        m_pImage->setCurrentIndex(iIndex);
    updateControls();
}

QString UIPhoneManager::selectedId() const
{
    QTableWidgetItem *pItem = m_pTable->item(m_pTable->currentRow(), 0);
    return pItem ? pItem->data(Qt::UserRole).toString() : QString();
}

void UIPhoneManager::updateDevices()
{
    const QString strSelected = selectedId();
    const QSignalBlocker blocker(m_pTable);
    const QList<UIPhoneDevice> devices = m_pBackend->devices();
    m_pTable->setRowCount(devices.size());
    int iSelected = -1;
    for (int i = 0; i < devices.size(); ++i)
    {
        const UIPhoneDevice &device = devices.at(i);
        QTableWidgetItem *pName = new QTableWidgetItem(device.m_strName);
        pName->setData(Qt::UserRole, device.m_strId);
        m_pTable->setItem(i, 0, pName);
        const QString strStatus = device.m_fBooted ? tr("Ready")
                                : !device.m_strProblem.isEmpty() ? tr("Needs attention")
                                : device.m_fStarting ? tr("Booting") : tr("Stopped");
        QTableWidgetItem *pStatus = new QTableWidgetItem(strStatus);
        pStatus->setToolTip(device.m_strProblem.isEmpty() ? device.m_strSerial : device.m_strProblem);
        m_pTable->setItem(i, 1, pStatus);
        if (device.m_strId == strSelected)
            iSelected = i;
    }
    if (!devices.isEmpty())
        m_pTable->selectRow(iSelected >= 0 ? iSelected : 0);
    updateControls();
}

void UIPhoneManager::updateControls()
{
    QString strError;
    const bool fIdle = !m_pBackend->isBusy();
    const bool fConfigured = m_pBackend->isConfigured(strError);
    const bool fApplied = m_pSdkPath->text() == m_pBackend->sdkRoot()
                       && m_pJavaHome->text() == m_pBackend->javaHome();
    const QString strId = selectedId();
    UIPhoneDevice selected;
    foreach (const UIPhoneDevice &device, m_pBackend->devices())
        if (device.m_strId == strId)
            selected = device;
    const bool fStopped = !strId.isEmpty() && selected.m_strSerial.isEmpty() && !selected.m_fStarting;
    foreach (QWidget *pWidget, QList<QWidget*>() << m_pSdkPath << m_pJavaHome << m_pBrowseSdk << m_pBrowseJava << m_pApply)
        pWidget->setEnabled(fIdle);
    m_pCheck->setEnabled(fIdle && fConfigured && fApplied);
    m_pRefresh->setEnabled(fIdle && fConfigured);
    m_pStart->setEnabled(fIdle && fConfigured && fApplied && fStopped);
    m_pColdBoot->setEnabled(m_pStart->isEnabled());
    m_pStop->setEnabled(fIdle && fConfigured && !selected.m_strSerial.isEmpty());
    m_pInstall->setEnabled(fIdle && fConfigured && selected.m_fBooted);
    for (QAction *pAction : {m_pBack, m_pHome, m_pRecent})
        pAction->setEnabled(m_pInstall->isEnabled());
    m_pLog->setEnabled(!strId.isEmpty() && QFileInfo(m_pBackend->dataRoot() + '/' + strId + ".log").isFile());
    m_pCreate->setEnabled(fIdle && fConfigured && fApplied && m_pImage->count() > 0 && !m_pName->text().trimmed().isEmpty());
    const bool fHasPhones = m_pTable->rowCount() > 0;
    const bool fHasImages = m_pImage->count() > 0;
    m_pTable->setVisible(fHasPhones);
    m_pPhoneActions->setVisible(fHasPhones);
    m_pEmptyCard->setVisible(!fHasPhones);
    m_pStart->setVisible(selected.m_strSerial.isEmpty());
    m_pStop->setVisible(!selected.m_strSerial.isEmpty());
    m_pEmptyAction->setEnabled(fIdle);
    m_pEmptyAction->setText(fHasImages ? tr("Create a phone") : tr("Set up SDK"));
    m_pEmptyTitle->setText(fHasImages ? tr("Create your first phone") : tr("Set up Android"));
    m_pEmpty->setText(fHasImages ? tr("Choose an Android image and a device configuration.")
                                 : tr("Connect an Android SDK with a system image to get started."));
    m_pRuntimeState->setText(fHasImages ? tr("Android runtime ready")
                                       : fConfigured ? tr("Android image needed") : tr("SDK setup required"));
    m_pCreateHelp->setText(fHasImages ? tr("Each phone has its own apps and storage.")
                                      : tr("Add an Android image in SDK Settings to continue."));
}

void UIPhoneManager::refresh()
{
    QString strError;
    if (!m_pBackend->isBusy() && m_pBackend->isConfigured(strError))
        m_pBackend->refresh();
}

void UIPhoneManager::createPhone()
{
    UIPhoneOptions options;
    options.m_strName = m_pName->text();
    options.m_strPackage = m_pImage->currentData().toString();
    options.m_iMemoryMB = m_pMemory->value();
    options.m_cCpus = m_pCpus->value();
    options.m_iStorageGB = m_pStorage->currentData().toInt();
    const QSize size = m_pDisplay->currentData().toSize();
    options.m_iWidth = size.width();
    options.m_iHeight = size.height();
    options.m_iDensity = size.width() == 720 ? 320 : 420;
    m_pBackend->createPhone(options);
}
