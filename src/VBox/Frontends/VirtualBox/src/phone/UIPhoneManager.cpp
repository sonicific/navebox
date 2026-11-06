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

/* Qt includes: */
#include <QAction>
#include <QComboBox>
#include <QDesktopServices>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStyledItemDelegate>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIDialog.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UIIconPool.h"
#include "UIManagerStyle.h"
#include "UIPhoneBackend.h"
#include "UIPhoneManager.h"

/** @page pg_navebox_phone_manager Navebox Android Phone Manager
 *
 * The Phones tool opens an SDK-backed phone catalog. Phone
 * data is separate from VirtualBox VMs and from Android Studio's AVD catalog.
 * Leaving this page or closing Navebox leaves detached emulator windows running.
 */

enum
{
    PhoneItemIdRole = Qt::UserRole,
    PhoneItemStateIconRole,
    PhoneItemStateTextRole
};

/** Paints two-line phone entries using the same selection surface as Machines. */
class QIPhoneItemDelegate : public QStyledItemDelegate
{
public:
    explicit QIPhoneItemDelegate(QObject *pParent) : QStyledItemDelegate(pParent) {}

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const RT_OVERRIDE
    {
        return QSize(180, qMax(50, option.fontMetrics.height() * 2 + 18));
    }

    virtual void paint(QPainter *pPainter, const QStyleOptionViewItem &option, const QModelIndex &index) const RT_OVERRIDE
    {
        QStyleOptionViewItem item(option);
        initStyleOption(&item, index);
        const bool fSelected = item.state.testFlag(QStyle::State_Selected);
        const QPalette palette = item.palette;
        const QRect card = item.rect.adjusted(4, 3, -4, -3);
        const int iIconSize = 28;
        const QRect iconRect(card.left() + 10, card.center().y() - iIconSize / 2, iIconSize, iIconSize);
        const int iTextLeft = iconRect.right() + 9;
        const int iTextWidth = qMax(0, card.right() - iTextLeft - 7);
        const int iLineHeight = item.fontMetrics.height();
        const int iTextSpacing = 3;
        const int iTextTop = card.top() + (card.height() - 2 * iLineHeight - iTextSpacing) / 2;
        const QRect nameRect(iTextLeft, iTextTop, iTextWidth, iLineHeight);
        const QRect statusRect(iTextLeft, nameRect.bottom() + 1 + iTextSpacing, iTextWidth, iLineHeight);
        const QRect statusIcon(statusRect.left(), statusRect.center().y() - 7, 14, 14);
        const QRect statusText = statusRect.adjusted(19, 0, 0, 0);

        pPainter->save();
        pPainter->setRenderHint(QPainter::Antialiasing);
        pPainter->setPen(Qt::NoPen);
        pPainter->setBrush(UIManagerStyle::machineBackgroundColor(palette, fSelected,
                           item.state.testFlag(QStyle::State_MouseOver) ? 100 : 0));
        pPainter->drawRoundedRect(card, 8, 8);
        item.icon.paint(pPainter, iconRect);
        index.data(PhoneItemStateIconRole).value<QIcon>().paint(pPainter, statusIcon);
        pPainter->setPen(palette.color(QPalette::Active, fSelected ? QPalette::HighlightedText : QPalette::WindowText));
        QFont font(item.font);
        font.setBold(true);
        pPainter->setFont(font);
        pPainter->drawText(nameRect, Qt::AlignLeading | Qt::AlignVCenter,
                           QFontMetrics(font).elidedText(item.text, Qt::ElideRight, nameRect.width()));
        pPainter->setFont(item.font);
        pPainter->drawText(statusText, Qt::AlignLeading | Qt::AlignVCenter,
                           item.fontMetrics.elidedText(index.data(PhoneItemStateTextRole).toString(),
                                                       Qt::ElideRight, statusText.width()));
        pPainter->restore();
    }
};

UIPhoneManager::UIPhoneManager(QWidget *pParent, const QString &strDataRoot)
    : QWidget(pParent)
    , m_pBackend(new UIPhoneBackend(this, strDataRoot))
{
    setObjectName("androidPhoneManager");
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->setSpacing(0);
    prepareActions();
    preparePhones(pLayout);
    prepareStatus(pLayout);
    prepareCreation();
    prepareSdk();
    prepareAppearance();

    /* Return in a path or name field must not start a phone. */
    foreach (QPushButton *pButton, findChildren<QPushButton*>())
        pButton->setAutoDefault(false);
    prepareConnections();
    retranslateUi();
    updateImages();
}

QList<QAction*> UIPhoneManager::toolbarActions() const
{
    return {m_pCreateAction, m_pSdkAction, m_pToolbarSeparator, m_pStart, m_pStop, m_pInstall, m_pRefresh, m_pMore};
}

void UIPhoneManager::prepareActions()
{
    m_pCreateAction = new QAction(UIIconPool::iconSetFull(":/vm_new_32px.png", ":/vm_new_16px.png"), QString(), this);
    m_pCreateAction->setObjectName("phoneCreateAction");
    m_pSdkAction = new QAction(UIIconPool::iconSetFull(":/global_settings_32px.png", ":/global_settings_16px.png"), QString(), this);
    m_pSdkAction->setObjectName("phoneSdkAction");
    m_pToolbarSeparator = new QAction(this);
    m_pToolbarSeparator->setSeparator(true);
    m_pStart = new QAction(UIIconPool::iconSetFull(":/vm_start_32px.png", ":/vm_start_16px.png",
                                                    ":/vm_start_disabled_32px.png", ":/vm_start_disabled_16px.png"),
                           QString(), this);
    m_pStart->setObjectName("phoneStart");
    m_pStop = new QAction(UIIconPool::iconSet(":/vm_poweroff_16px.png"), QString(), this);
    m_pStop->setObjectName("phoneStop");
    m_pInstall = new QAction(UIIconPool::iconSetFull(":/extension_pack_install_32px.png", ":/extension_pack_install_16px.png"),
                             QString(), this);
    m_pInstall->setObjectName("phoneInstallApk");
    m_pRefresh = new QAction(UIIconPool::iconSetFull(":/refresh_32px.png", ":/refresh_16px.png"), QString(), this);
    QMenu *pMore = new QMenu(this);
    m_pMore = pMore->menuAction();
    m_pMore->setIcon(UIIconPool::iconSet(":/tools_menu_24px.png"));
    m_pColdBoot = pMore->addAction(QString());
    m_pSoftwareBoot = pMore->addAction(QString());
    m_pLog = pMore->addAction(QString());
    pMore->addSeparator();
    m_pBack = pMore->addAction(QString());
    m_pHome = pMore->addAction(QString());
    m_pRecent = pMore->addAction(QString());
}

void UIPhoneManager::prepareAppearance()
{
    /* Scope the complete control styling to this page so secondary buttons
     * use the same frame and spacing as primary actions on native Windows. */
    setStyleSheet(QStringLiteral(
        "QFrame[phoneCard=\"true\"] { background: palette(base); border: 1px solid palette(midlight); border-radius: 10px; }"
        "QLabel[phoneSection=\"true\"] { font-size: 15px; font-weight: 600; }"
        "QFrame[phoneDetailsCard=\"true\"] { background: palette(base); border: 1px solid %1; border-radius: 8px; }"
        "QWidget[phoneDetailsTitle=\"true\"] { border-bottom: 1px solid %1; }"
        "QPushButton { background: palette(base); color: palette(button-text);"
        " border: 1px solid palette(midlight); border-radius: 6px; padding: 4px 12px; }"
        "QPushButton:hover { background: palette(alternate-base); border-color: palette(mid); }"
        "QPushButton:pressed { background: palette(button); }"
        "QPushButton:focus { border-color: palette(highlight); }"
        "QPushButton:disabled { background: palette(window); color: palette(mid); }"
        "QLineEdit, QSpinBox, QComboBox { background: palette(base); color: palette(text);"
        " border: 1px solid palette(midlight); border-radius: 6px; padding: 4px 10px; }"
        "QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: palette(highlight); }"
        "QSpinBox QLineEdit { background: transparent; border: none; padding: 0; }"
        "QComboBox::drop-down { border: none; background: transparent; width: 22px; }"
        "QSpinBox::up-button, QSpinBox::down-button { border: none; background: transparent; width: 20px; }"
        "QSplitter::handle { background: %1; }"
        "QListWidget#phoneList { background: palette(window); border: none; }"
        "QFrame#phonePreviewDevice { border: 2px solid palette(highlight); border-radius: 18px; }"
        "QLabel#phonePreviewScreen { color: palette(highlight); font-weight: 600; }"
        "QPushButton[phonePrimary=\"true\"] { background: palette(highlight); color: palette(highlighted-text);"
        " border: 1px solid palette(highlight); font-weight: 600; }"
        "QPushButton[phonePrimary=\"true\"]:focus { border-color: palette(window-text); }"
        "QPushButton[phonePrimary=\"true\"]:disabled { background: palette(button); color: palette(button-text);"
        " border-color: palette(midlight); }"
        "QPushButton[phonePrimary=\"true\"]:hover { border-color: palette(window-text); }")
        .arg(UIManagerStyle::borderColor(palette()).name(QColor::HexArgb)));
}

void UIPhoneManager::prepareSdk()
{
    m_pSdkDialog = new QIDialog(this);
    m_pSdkDialog->setObjectName("phoneSdkPanel");
    m_pSdkDialog->setMinimumWidth(680);
    QVBoxLayout *pViewLayout = new QVBoxLayout(m_pSdkDialog);
    QFrame *pSdkCard = new QFrame(m_pSdkDialog);
    pSdkCard->setProperty("phoneCard", true);
    pViewLayout->addWidget(pSdkCard);
    QVBoxLayout *pSdkLayout = new QVBoxLayout(pSdkCard);
    pSdkLayout->setContentsMargins(16, 14, 16, 16);
    pSdkLayout->setSpacing(14);
    m_pSdkTitle = new QLabel(pSdkCard);
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
                                                            : m_pBackend->sdkRoot(), pSdkCard);
    m_pSdkPath->setObjectName("phoneSdkPath");
    m_pJavaHome = new QLineEdit(m_pBackend->javaHome().isEmpty() ? UIPhoneBackend::suggestedJavaHome()
                                                             : m_pBackend->javaHome(), pSdkCard);
    m_pJavaHome->setObjectName("phoneJavaHome");
    m_pBrowseSdk = new QPushButton(pSdkCard);
    m_pBrowseSdk->setObjectName("phoneBrowseSdk");
    m_pBrowseJava = new QPushButton(pSdkCard);
    m_pBrowseJava->setObjectName("phoneBrowseJava");
    QHBoxLayout *pSdkRow = new QHBoxLayout;
    pSdkRow->setSpacing(8);
    pSdkRow->addWidget(m_pSdkPath, 1);
    pSdkRow->addWidget(m_pBrowseSdk);
    m_pSdkForm->addRow(new QLabel(pSdkCard), pSdkRow);
    QHBoxLayout *pJavaRow = new QHBoxLayout;
    pJavaRow->setSpacing(8);
    pJavaRow->addWidget(m_pJavaHome, 1);
    pJavaRow->addWidget(m_pBrowseJava);
    m_pSdkForm->addRow(new QLabel(pSdkCard), pJavaRow);
    m_pApply = new QPushButton(pSdkCard);
    m_pApply->setObjectName("phoneApplySdk");
    m_pCheck = new QPushButton(pSdkCard);
    m_pCheck->setObjectName("phoneCheckAcceleration");
    m_pHelp = new QPushButton(pSdkCard);
    m_pHelp->setObjectName("phoneSetupHelp");
    QHBoxLayout *pSetupActions = new QHBoxLayout;
    pSetupActions->setSpacing(8);
    pSetupActions->addWidget(m_pApply);
    pSetupActions->addWidget(m_pCheck);
    pSetupActions->addStretch();
    pSetupActions->addWidget(m_pHelp);
    m_pSdkDone = new QPushButton(pSdkCard);
    m_pSdkDone->setObjectName("phoneSdkDone");
    pSetupActions->addWidget(m_pSdkDone);
    m_pSdkForm->addRow(pSetupActions);
    m_pSdkNotice = new QLabel(m_pSdkDialog);
    m_pSdkNotice->setTextFormat(Qt::PlainText);
    m_pSdkNotice->setWordWrap(true);
    m_pSdkNotice->hide();
    pViewLayout->addWidget(m_pSdkNotice);
}

void UIPhoneManager::preparePhones(QVBoxLayout *pLayout)
{
    QFrame *pEmptyCard = new QFrame(this);
    m_pEmptyCard = pEmptyCard;
    QVBoxLayout *pEmptyLayout = new QVBoxLayout(pEmptyCard);
    pEmptyLayout->setContentsMargins(24, 22, 24, 22);
    pEmptyLayout->setSpacing(10);
    pEmptyLayout->addStretch();
    QLabel *pSymbol = new QLabel(pEmptyCard);
    pSymbol->setPixmap(UIIconPool::iconSet(":/navebox-phone.svg").pixmap(56, 56));
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
    QSplitter *pSplitter = new QSplitter(Qt::Horizontal, this);
    m_pDeviceView = pSplitter;
    pSplitter->setChildrenCollapsible(false);
    pSplitter->setHandleWidth(1);
    m_pList = new QListWidget(pSplitter);
    m_pList->setObjectName("phoneList");
    m_pList->setMinimumWidth(180);
    m_pList->setUniformItemSizes(true);
    m_pList->setMouseTracking(true);
    m_pList->setItemDelegate(new QIPhoneItemDelegate(m_pList));
    m_pList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pSplitter->addWidget(m_pList);

    QScrollArea *pScroll = new QScrollArea(pSplitter);
    pScroll->setFrameShape(QFrame::NoFrame);
    pScroll->setWidgetResizable(true);
    QWidget *pDetails = new QWidget;
    QVBoxLayout *pDetailLayout = new QVBoxLayout(pDetails);
    const int iInset = UIManagerStyle::detailsPaneInset();
    pDetailLayout->setContentsMargins(iInset, 4, iInset, 0);
    m_pNoSelection = new QLabel(pDetails);
    m_pNoSelection->setAlignment(Qt::AlignCenter);
    m_pNoSelection->setWordWrap(true);
    pDetailLayout->addWidget(m_pNoSelection, 1);
    m_pSelectedDetails = new QWidget(pDetails);
    QVBoxLayout *pSections = new QVBoxLayout(m_pSelectedDetails);
    pSections->setContentsMargins(0, 0, 0, 0);
    pSections->setSpacing(8);
    preparePhoneDetails(pSections);
    pDetailLayout->addWidget(m_pSelectedDetails, 1, Qt::AlignTop);
    pScroll->setWidget(pDetails);
    pSplitter->addWidget(pScroll);
    pSplitter->setSizes({250, 750});
    pLayout->addWidget(pSplitter, 1);
    pLayout->addWidget(pEmptyCard, 1);
}

void UIPhoneManager::preparePhoneDetails(QVBoxLayout *pLayout)
{
    const auto addSection = [pLayout](const char *pszName, const char *pszIcon, int cRows)
    {
        QFrame *pCard = new QFrame;
        pCard->setProperty("phoneDetailsCard", true);
        QVBoxLayout *pCardLayout = new QVBoxLayout(pCard);
        pCardLayout->setContentsMargins(1, 1, 1, 1);
        pCardLayout->setSpacing(0);
        QWidget *pHeader = new QWidget(pCard);
        pHeader->setProperty("phoneDetailsTitle", true);
        QHBoxLayout *pHeaderLayout = new QHBoxLayout(pHeader);
        pHeaderLayout->setContentsMargins(10, 5, 10, 5);
        pHeaderLayout->setSpacing(6);
        QLabel *pIcon = new QLabel(pHeader);
        pIcon->setPixmap(UIIconPool::iconSet(QLatin1String(pszIcon)).pixmap(16, 16));
        pHeaderLayout->addWidget(pIcon);
        QLabel *pTitle = new QLabel(pHeader);
        pTitle->setObjectName(QLatin1String(pszName));
        QFont font = pTitle->font();
        font.setBold(true);
        pTitle->setFont(font);
        pHeaderLayout->addWidget(pTitle, 1);
        pCardLayout->addWidget(pHeader);
        QFormLayout *pForm = new QFormLayout;
        pForm->setContentsMargins(10, 6, 10, 6);
        pForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignTop);
        pForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        pForm->setHorizontalSpacing(12);
        pForm->setVerticalSpacing(2);
        pCardLayout->addLayout(pForm);
        for (int i = 0; i < cRows; ++i)
        {
            QLabel *pValue = new QLabel(pCard);
            pValue->setTextFormat(Qt::PlainText);
            pValue->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
            pValue->setWordWrap(true);
            pForm->addRow(new QLabel(pCard), pValue);
        }
        pLayout->addWidget(pCard);
        return pForm;
    };
    m_pIdentity = addSection("phoneIdentityHeading", ":/machine_16px.png", 5);
    m_pHardware = addSection("phoneHardwareHeading", ":/chipset_16px.png", 3);
    m_pScreen = addSection("phoneDisplayHeading", ":/display_hardware_16px.png", 1);
    m_pDisk = addSection("phoneStorageHeading", ":/hd_16px.png", 1);
    m_pIdentity->itemAt(0, QFormLayout::FieldRole)->widget()->setObjectName("phoneSelectedName");
    m_pHardware->itemAt(1, QFormLayout::FieldRole)->widget()->setObjectName("phoneSelectedMemory");
}

void UIPhoneManager::prepareCreation()
{
    m_pCreateDialog = new QIDialog(this);
    m_pCreateDialog->setObjectName("phoneCreateDialog");
    m_pCreateDialog->setMinimumWidth(620);
    QWidget *pCreate = m_pCreateDialog;
    QVBoxLayout *pCreateLayout = new QVBoxLayout(pCreate);
    pCreateLayout->setSpacing(12);
    QHBoxLayout *pCards = new QHBoxLayout;
    pCards->setSpacing(14);
    pCreateLayout->addLayout(pCards);
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
    m_pDisplay->setObjectName("phoneProfile");
    for (const UIPhoneProfile &profile : UIPhoneBackend::profiles())
        m_pDisplay->addItem(QString(), QVariant::fromValue(QSize(profile.m_iWidth, profile.m_iHeight)));
    m_pDisplay->addItem(QString());
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
    m_pCreateNotice = new QLabel(pCreate);
    m_pCreateNotice->setTextFormat(Qt::PlainText);
    m_pCreateNotice->setWordWrap(true);
    m_pCreateNotice->hide();
    pCreateLayout->addWidget(m_pCreateNotice);
    QHBoxLayout *pCreateActions = new QHBoxLayout;
    m_pCreateHelp = new QLabel(pCreate);
    m_pCreateHelp->setWordWrap(true);
    pCreateActions->addWidget(m_pCreateHelp, 1);
    m_pCancel = new QPushButton(pCreate);
    m_pCancel->setObjectName("phoneCreateCancel");
    pCreateActions->addWidget(m_pCancel);
    m_pCreate = new QPushButton(pCreate);
    m_pCreate->setObjectName("phoneCreate");
    m_pCreate->setProperty("phonePrimary", true);
    pCreateActions->addWidget(m_pCreate);
    pCreateLayout->addLayout(pCreateActions);
    pCreateLayout->addStretch(1);
}

void UIPhoneManager::prepareStatus(QVBoxLayout *pLayout)
{
    QHBoxLayout *pStatus = new QHBoxLayout;
    pStatus->setSpacing(8);
    m_pNotice = new QLabel(this);
    m_pNotice->setObjectName("phoneNotice");
    m_pNotice->setTextFormat(Qt::PlainText);
    m_pNotice->setWordWrap(true);
    pStatus->addWidget(m_pNotice, 1);
    m_pNotice->hide();
    m_pDetails = new QPushButton(this);
    m_pDetails->setObjectName("phoneDetails");
    m_pDetails->setCheckable(true);
    pStatus->addWidget(m_pDetails);
    m_pDetails->hide();
    pLayout->addLayout(pStatus);
    m_pResult = new QPlainTextEdit(this);
    m_pResult->setObjectName("phoneResult");
    m_pResult->setReadOnly(true);
    m_pResult->setMaximumBlockCount(200);
    m_pResult->setMaximumHeight(110);
    pLayout->addWidget(m_pResult);
    m_pResult->hide();
}

void UIPhoneManager::prepareConnections()
{
    connect(m_pCreateAction, &QAction::triggered, this, &UIPhoneManager::showCreateDialog);
    connect(m_pSdkAction, &QAction::triggered, this, &UIPhoneManager::showSdkSettings);
    connect(m_pSdkDone, &QPushButton::clicked, m_pSdkDialog, &QDialog::accept);
    connect(m_pCancel, &QPushButton::clicked, m_pCreateDialog, &QDialog::reject);
    connect(m_pDetails, &QPushButton::toggled, m_pResult, &QWidget::setVisible);
    connect(m_pEmptyAction, &QPushButton::clicked, this, [this]()
    {
        if (m_pImage->count() > 0)
        {
            m_pCreateAction->trigger();
        }
        else
        {
            showSdkSettings();
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
    connect(m_pRefresh, &QAction::triggered, this, &UIPhoneManager::refresh);
    connect(m_pStart, &QAction::triggered, this, [this]() { m_pBackend->startPhone(selectedId()); });
    connect(m_pColdBoot, &QAction::triggered, this, [this]() { m_pBackend->startPhone(selectedId(), true); });
    connect(m_pSoftwareBoot, &QAction::triggered, this, [this]() { m_pBackend->startPhone(selectedId(), true, true); });
    connect(m_pStop, &QAction::triggered, this, [this]() { m_pBackend->stopPhone(selectedId()); });
    connect(m_pInstall, &QAction::triggered, this, [this]()
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
    connect(m_pList, &QListWidget::itemSelectionChanged, this, [this]()
    {
        updatePhoneDetails();
        updateControls();
    });
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
        m_pCreateDialog->accept();
        for (int iRow = 0; iRow < m_pList->count(); ++iRow)
            if (m_pList->item(iRow)->data(PhoneItemIdRole).toString() == strId)
            {
                m_pList->setCurrentRow(iRow);
                break;
            }
    });
    QTimer *pPoll = new QTimer(this);
    pPoll->setInterval(5000);
    connect(pPoll, &QTimer::timeout, this, [this]()
    {
        if (isVisible())
            refresh();
    });
    pPoll->start();
}

void UIPhoneManager::changeEvent(QEvent *pEvent)
{
    QWidget::changeEvent(pEvent);
    if (pEvent->type() == QEvent::LanguageChange)
        retranslateUi();
    else if (pEvent->type() == QEvent::PaletteChange)
        prepareAppearance();
    else if (pEvent->type() == QEvent::FontChange || pEvent->type() == QEvent::StyleChange)
        updateControlGeometry();
}

void UIPhoneManager::showEvent(QShowEvent *pEvent)
{
    QWidget::showEvent(pEvent);
    refresh();
}

void UIPhoneManager::updateControlGeometry()
{
    QList<QWidget*> controls = QList<QWidget*>() << m_pSdkPath << m_pJavaHome << m_pName << m_pImage
                                                << m_pMemory << m_pCpus << m_pStorage << m_pDisplay;
    foreach (QPushButton *pButton, findChildren<QPushButton*>())
    {
        pButton->setMinimumWidth(qMax(88, pButton->fontMetrics().horizontalAdvance(pButton->text()) + 28));
        controls << pButton;
    }
    /* Give every field and action one shared height, including hidden dialogs.
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
    const int cActionWidth = qMax(108, qMax(m_pCreate->minimumWidth(), m_pDetails->minimumWidth()));
    for (QPushButton *pButton : {m_pCreate, m_pDetails})
        pButton->setFixedWidth(cActionWidth);
    m_pMemory->setMinimumWidth(m_pMemory->fontMetrics().horizontalAdvance(QString::number(m_pMemory->maximum())
                                                                        + m_pMemory->suffix()) + 42);
    m_pCpus->setMinimumWidth(m_pCpus->fontMetrics().horizontalAdvance(QString::number(m_pCpus->maximum())) + 42);
}

void UIPhoneManager::retranslateUi()
{
    setAccessibleName(tr("Android Phones"));
    m_pCreateAction->setText(tr("Create Phone"));
    m_pSdkAction->setText(tr("SDK Settings"));
    m_pCreateDialog->setWindowTitle(m_pCreateAction->text());
    m_pSdkDialog->setWindowTitle(m_pSdkAction->text());
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
    m_pSdkDone->setText(tr("Done"));
    m_pCancel->setText(tr("Cancel"));
    findChild<QLabel*>("phoneIdentityHeading")->setText(tr("General"));
    findChild<QLabel*>("phoneHardwareHeading")->setText(tr("System"));
    findChild<QLabel*>("phoneDisplayHeading")->setText(tr("Display"));
    findChild<QLabel*>("phoneStorageHeading")->setText(tr("Storage"));
    m_pList->setAccessibleName(tr("Android phones"));
    m_pNoSelection->setText(tr("Select a phone to view its details."));
    m_pRefresh->setText(tr("Refresh"));
    m_pStart->setText(tr("Start"));
    m_pColdBoot->setText(tr("Cold Boot"));
    m_pColdBoot->setToolTip(tr("Start without loading a Quick Boot snapshot; user data is preserved."));
    m_pSoftwareBoot->setText(tr("Cold Boot with Software Graphics"));
    m_pSoftwareBoot->setToolTip(tr("Use software graphics for this session without loading or saving a Quick Boot snapshot."));
    m_pStop->setText(tr("Stop"));
    m_pInstall->setText(tr("Install APK..."));
    m_pMore->setText(tr("More"));
    m_pMore->setToolTip(tr("More phone actions"));
    m_pBack->setText(tr("Back"));
    m_pHome->setText(tr("Home"));
    m_pRecent->setText(tr("Recent Apps"));
    m_pLog->setText(tr("Open Log"));
    const QStringList labels = QStringList() << tr("Phone name:") << tr("Android image:") << tr("Memory:")
                                             << tr("CPUs:") << tr("Storage:") << tr("Device profile:");
    for (int i = 0; i < labels.size(); ++i)
        qobject_cast<QLabel*>(m_pCreateForm->itemAt(i, QFormLayout::LabelRole)->widget())->setText(labels.at(i));
    m_pName->setPlaceholderText(tr("My Android Phone"));
    m_pMemory->setSuffix(tr(" MB"));
    m_pCreateTitle->setText(tr("Phone configuration"));
    m_pPreviewTitle->setText(tr("Device preview"));
    m_pPreviewScreen->setText(tr("Android"));
    const QList<UIPhoneProfile> profiles = UIPhoneBackend::profiles();
    for (int i = 0; i < profiles.size(); ++i)
    {
        const UIPhoneProfile &profile = profiles.at(i);
        m_pDisplay->setItemText(i, tr("%1 x %2 (%3 dpi)").arg(profile.m_iWidth).arg(profile.m_iHeight).arg(profile.m_iDensity));
    }
    m_pDisplay->setItemText(profiles.size(), tr("Vary for testing"));
    m_pDisplay->setToolTip(tr("Vary for testing assigns a display and SDK hardware profile when the phone is created."));
    m_pCreate->setText(tr("Create Phone"));
    m_pResult->setAccessibleName(tr("Phone operation result"));
    m_pDetails->setText(tr("Details"));
    updateControlGeometry();
    updatePreview();
    updateDevices();
}

void UIPhoneManager::updatePreview()
{
    const QSize size = m_pDisplay->currentData().toSize();
    m_pPreviewDisplay->setText(size.isValid() ? tr("%1 x %2 pixels").arg(size.width()).arg(size.height())
                                            : tr("Selected at creation"));
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
    m_pNotice->setVisible(!m_pNotice->text().isEmpty());
    m_pDetails->setChecked(false);
    m_pDetails->setVisible(fDetails);
    if (m_pSdkDialog->isVisible())
    {
        m_pSdkNotice->setText(m_pNotice->text());
        m_pSdkNotice->show();
    }
    if (m_pCreateDialog->isVisible() && !fSuccess)
    {
        m_pCreateNotice->setText(tr("Error: %1").arg(strText.left(500)));
        m_pCreateNotice->show();
    }
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

void UIPhoneManager::showCreateDialog()
{
    m_pCreateNotice->hide();
    m_pName->setFocus();
    m_pCreateDialog->open();
    gpDesktop->centerWidget(m_pCreateDialog, this);
}

void UIPhoneManager::showSdkSettings()
{
    m_pSdkNotice->hide();
    m_pSdkPath->setFocus();
    m_pSdkDialog->open();
    gpDesktop->centerWidget(m_pSdkDialog, this);
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
    const QList<QListWidgetItem*> selected = m_pList->selectedItems();
    return selected.isEmpty() ? QString() : selected.first()->data(PhoneItemIdRole).toString();
}

QString UIPhoneManager::deviceStatus(const UIPhoneDevice &device) const
{
    if (device.m_fBooted)
        return tr("Ready");
    if (!device.m_strProblem.isEmpty())
        return tr("Needs attention");
    return device.m_fStarting ? tr("Booting") : tr("Stopped");
}

void UIPhoneManager::updatePhoneDetails()
{
    const QString strId = selectedId();
    const UIPhoneDevice device = m_pBackend->device(strId);
    m_pSelectedDetails->setVisible(!device.m_strId.isEmpty());
    m_pNoSelection->setVisible(device.m_strId.isEmpty());
    if (device.m_strId.isEmpty())
        return;

    const UIPhoneConfiguration config = m_pBackend->configuration(strId);
    const QString strUnknown = tr("Unavailable");
    QString strImage = device.m_strPackage;
    const QStringList package = strImage.split(';');
    if (package.size() == 4 && package.at(0) == "system-images" && package.at(1).startsWith("android-"))
        strImage = tr("API %1 / %2 / %3").arg(package.at(1).mid(8), package.at(2), package.at(3));

    QString strStorage = config.m_strStorage;
    bool fGigabytes = false;
    if (strStorage.endsWith('G', Qt::CaseInsensitive))
    {
        const qulonglong cGigabytes = strStorage.chopped(1).toULongLong(&fGigabytes);
        if (fGigabytes)
            strStorage = tr("%1 GB").arg(cGigabytes);
    }
    QString strDisplay;
    if (config.m_iWidth > 0 && config.m_iHeight > 0)
    {
        strDisplay = tr("%1 x %2 pixels").arg(config.m_iWidth).arg(config.m_iHeight);
        if (config.m_iDensity > 0)
            strDisplay += tr(" (%1 dpi)").arg(config.m_iDensity);
    }

    const auto setRows = [&strUnknown](QFormLayout *pForm, const QStringList &labels, const QStringList &values)
    {
        for (int i = 0; i < labels.size(); ++i)
        {
            static_cast<QLabel*>(pForm->itemAt(i, QFormLayout::LabelRole)->widget())->setText(labels.at(i));
            const QString &strValue = values.at(i);
            static_cast<QLabel*>(pForm->itemAt(i, QFormLayout::FieldRole)->widget())
                ->setText(strValue.isEmpty() ? strUnknown : strValue);
        }
    };
    setRows(m_pIdentity, {tr("Name"), tr("Status"), tr("Android image"), tr("AVD ID"), tr("ADB serial")},
            {device.m_strName, deviceStatus(device), strImage, strId, device.m_strSerial});
    setRows(m_pHardware, {tr("SDK profile"), tr("Configured RAM"), tr("CPUs")},
            {config.m_strProfile, config.m_iMemoryMB > 0 ? tr("%1 MB").arg(config.m_iMemoryMB) : QString(),
             config.m_cCpus > 0 ? QString::number(config.m_cCpus) : QString()});
    setRows(m_pScreen, {tr("Resolution")}, {strDisplay});
    setRows(m_pDisk, {tr("Data partition")}, {strStorage});
}

void UIPhoneManager::updateDevices()
{
    const QString strSelected = selectedId();
    const QSignalBlocker blocker(m_pList);
    const QList<UIPhoneDevice> devices = m_pBackend->devices();
    while (m_pList->count() > devices.size())
        delete m_pList->takeItem(m_pList->count() - 1);
    int iSelected = -1;
    for (int i = 0; i < devices.size(); ++i)
    {
        const UIPhoneDevice &device = devices.at(i);
        QListWidgetItem *pItem = m_pList->item(i);
        if (!pItem)
            pItem = new QListWidgetItem(UIIconPool::iconSet(":/navebox-phone.svg"), QString(), m_pList);
        const QString strStatus = deviceStatus(device);
        pItem->setText(device.m_strName);
        pItem->setData(PhoneItemIdRole, device.m_strId);
        pItem->setData(PhoneItemStateTextRole, strStatus);
        const char *pszStateIcon = device.m_fBooted ? ":/state_running_16px.png"
                                 : !device.m_strProblem.isEmpty() ? ":/state_aborted_16px.png"
                                 : device.m_fStarting ? ":/state_loading_16px.png" : ":/state_powered_off_16px.png";
        pItem->setData(PhoneItemStateIconRole, UIIconPool::iconSet(QLatin1String(pszStateIcon)));
        pItem->setData(Qt::AccessibleTextRole, tr("%1, %2").arg(device.m_strName, strStatus));
        pItem->setToolTip(QString("<qt>%1</qt>").arg((device.m_strProblem.isEmpty() ? device.m_strName
                                                                                   : device.m_strProblem).toHtmlEscaped()));
        if (device.m_strId == strSelected)
            iSelected = i;
    }
    if (iSelected >= 0 && m_pList->currentRow() != iSelected)
        m_pList->setCurrentRow(iSelected);
    else if (iSelected < 0)
        m_pList->clearSelection();
    updatePhoneDetails();
    updateControls();
}

void UIPhoneManager::updateControls()
{
    QString strError;
    const bool fIdle = !m_pBackend->isBusy() || m_pBackend->isRefreshing();
    const bool fConfigured = m_pBackend->isConfigured(strError);
    const bool fApplied = m_pSdkPath->text() == m_pBackend->sdkRoot()
                       && m_pJavaHome->text() == m_pBackend->javaHome();
    const QString strId = selectedId();
    const UIPhoneDevice selected = m_pBackend->device(strId);
    const bool fHasSelection = !selected.m_strId.isEmpty();
    const bool fStopped = fHasSelection && selected.m_strSerial.isEmpty() && !selected.m_fStarting;
    foreach (QWidget *pWidget, QList<QWidget*>() << m_pSdkPath << m_pJavaHome << m_pBrowseSdk << m_pBrowseJava << m_pApply)
        pWidget->setEnabled(fIdle);
    m_pCheck->setEnabled(fIdle && fConfigured && fApplied);
    m_pRefresh->setEnabled(!m_pBackend->isBusy() && fConfigured);
    m_pStart->setEnabled(fIdle && fConfigured && fStopped);
    m_pColdBoot->setEnabled(m_pStart->isEnabled());
    m_pSoftwareBoot->setEnabled(m_pStart->isEnabled());
    m_pStop->setEnabled(fIdle && fConfigured && !selected.m_strSerial.isEmpty());
    m_pInstall->setEnabled(fIdle && fConfigured && selected.m_fBooted);
    for (QAction *pAction : {m_pBack, m_pHome, m_pRecent})
        pAction->setEnabled(m_pInstall->isEnabled());
    m_pLog->setEnabled(!strId.isEmpty() && QFileInfo(m_pBackend->dataRoot() + '/' + strId + ".log").isFile());
    m_pMore->setEnabled(fHasSelection && (m_pStart->isEnabled() || m_pInstall->isEnabled() || m_pLog->isEnabled()));
    m_pCreate->setEnabled(fIdle && fConfigured && m_pImage->count() > 0 && !m_pName->text().trimmed().isEmpty());
    const bool fHasPhones = m_pList->count() > 0;
    const bool fHasImages = m_pImage->count() > 0;
    m_pDeviceView->setVisible(fHasPhones);
    m_pEmptyCard->setVisible(!fHasPhones);
    m_pStart->setVisible(selected.m_strSerial.isEmpty());
    m_pStop->setVisible(!selected.m_strSerial.isEmpty());
    m_pEmptyAction->setEnabled(fIdle);
    m_pEmptyAction->setText(fHasImages ? tr("Create a phone") : tr("Set up SDK"));
    m_pEmptyTitle->setText(fHasImages ? tr("Create your first phone") : tr("Set up Android"));
    m_pEmpty->setText(fHasImages ? tr("Choose an Android image and a device configuration.")
                                 : tr("Connect an Android SDK with a system image to get started."));
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
    m_pCreateNotice->hide();
    UIPhoneOptions options;
    options.m_strName = m_pName->text();
    options.m_strPackage = m_pImage->currentData().toString();
    options.m_iMemoryMB = m_pMemory->value();
    options.m_cCpus = m_pCpus->value();
    options.m_iStorageGB = m_pStorage->currentData().toInt();
    const QSize size = m_pDisplay->currentData().toSize();
    options.m_fVaryProfile = !size.isValid();
    for (const UIPhoneProfile &profile : UIPhoneBackend::profiles())
        if (size == QSize(profile.m_iWidth, profile.m_iHeight))
        {
            options.m_iWidth = profile.m_iWidth;
            options.m_iHeight = profile.m_iHeight;
            options.m_iDensity = profile.m_iDensity;
            break;
        }
    m_pBackend->createPhone(options);
}
