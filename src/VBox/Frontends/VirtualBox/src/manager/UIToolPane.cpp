/* $Id: UIToolPane.cpp 114848 2026-08-04 09:49:02Z andreas.loeffler@oracle.com $ */
/** @file
 * VBox Qt GUI - UIToolPane class implementation.
 */

/*
 * Copyright (C) 2017-2026 Oracle and/or its affiliates.
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
#include <QApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedLayout>
#include <QStyle>
#include <QStylePainter>
#include <QVBoxLayout>

/* GUI includes */
#include "UICloudProfileManager.h"
#include "UICommon.h"
#include "UIDetails.h"
#include "UIErrorPane.h"
#include "UIExtensionPackManager.h"
#include "UIFileManager.h"
#include "UIGlobalSession.h"
#include "UIHomePane.h"
#include "UIMachineToolsWidget.h"
#include "UIManagerStyle.h"
#include "UIMediumManager.h"
#include "UINetworkManager.h"
#include "UIPhoneManager.h"
#include "UISnapshotPane.h"
#include "UIToolPane.h"
#include "UIToolsItem.h"
#include "UITranslationEventListener.h"
#include "UIVirtualMachineItem.h"
#include "UIVMActivityOverviewWidget.h"
#include "UIVMActivityToolWidget.h"
#include "UIVMLogViewerWidget.h"

/* Other VBox includes: */
#include <iprt/assert.h>


UIToolPane::UIToolPane(QWidget *pParent, UIToolClass enmClass, UIActionPool *pActionPool)
    : QWidget(pParent)
    , m_enmClass(enmClass)
    , m_pActionPool(pActionPool)
    , m_fActive(false)
    , m_pLayout(0)
    // Global tools
    , m_pToolBar(0)
    , m_pPaneHome(0)
    , m_pPaneMachines(0)
    , m_pPanePhones(0)
    , m_pPaneExtensions(0)
    , m_pPaneMedia(0)
    , m_pPaneNetwork(0)
    , m_pPaneCloud(0)
    , m_pPaneVMActivityOverview(0)
    // Machine tools
    , m_pPaneError(0)
    , m_pPaneDetails(0)
    , m_pPaneSnapshots(0)
    , m_pPaneLogViewer(0)
    , m_pPaneVMActivityTool(0)
    , m_pPaneFileManager(0)
{
    prepare();
}

UIToolPane::~UIToolPane()
{
    cleanup();
}

void UIToolPane::setActive(bool fActive)
{
    /* Save activity: */
    if (m_fActive != fActive)
    {
        m_fActive = fActive;

        /* Handle token change: */
        handleTokenChange();
    }
}

UIToolType UIToolPane::currentTool() const
{
    return   m_pLayout && m_pLayout->currentWidget()
           ? m_pLayout->currentWidget()->property("ToolType").value<UIToolType>()
           : UIToolType_Invalid;
}

bool UIToolPane::isToolOpened(UIToolType enmType) const
{
    for (int iIndex = 0; iIndex < m_pLayout->count(); ++iIndex)
        if (m_pLayout->widget(iIndex)->property("ToolType").value<UIToolType>() == enmType)
            return true;
    return false;
}

void UIToolPane::openTool(UIToolType enmType)
{
    if (enmType == UIToolType_Phones)
        phoneManager();

    /* Search through the stacked widgets: */
    int iActualIndex = -1;
    for (int iIndex = 0; iIndex < m_pLayout->count(); ++iIndex)
        if (m_pLayout->widget(iIndex)->property("ToolType").value<UIToolType>() == enmType)
            iActualIndex = iIndex;

    /* If widget with such type exists: */
    if (iActualIndex != -1)
    {
        /* Activate corresponding index: */
        m_pLayout->setCurrentIndex(iActualIndex);
    }
    /* Otherwise: */
    else
    {
        /* Create, remember, append corresponding stacked widget: */
        switch (enmType)
        {
            case UIToolType_Home:
            {
                /* Create Home pane: */
                m_pPaneHome = new UIHomePane;
                AssertPtrReturnVoid(m_pPaneHome);
                {
                    /* Configure pane: */
                    m_pPaneHome->setProperty("ToolType", QVariant::fromValue(UIToolType_Home));
                    connect(m_pPaneHome, &UIHomePane::sigHomeTask, this, &UIToolPane::sigHomeTask);

                    /* Add into layout: */
                    addToolWidget(m_pPaneHome);
                }
                break;
            }
            case UIToolType_Machines:
            {
                /* Create Machine Tools Widget: */
                m_pPaneMachines = new UIMachineToolsWidget(this, m_pActionPool);
                AssertPtrReturnVoid(m_pPaneMachines);
                {
                    /* Configure pane: */
                    m_pPaneMachines->setProperty("ToolType", QVariant::fromValue(UIToolType_Machines));
                    /// @todo connect!

                    /* Add into layout: */
                    addToolWidget(m_pPaneMachines);
                }
                break;
            }
            case UIToolType_Extensions:
            {
                /* Create Extension Pack Manager: */
                m_pPaneExtensions = new UIExtensionPackManagerWidget(EmbedTo_Stack, m_pActionPool, false /* show toolbar */);
                AssertPtrReturnVoid(m_pPaneExtensions);
                {
                    /* Configure pane: */
                    m_pPaneExtensions->setProperty("ToolType", QVariant::fromValue(UIToolType_Extensions));

                    /* Add into layout: */
                    addToolWidget(m_pPaneExtensions);
                }
                break;
            }
            case UIToolType_Media:
            {
                /* Create Virtual Media Manager: */
                m_pPaneMedia = new UIMediumManagerWidget(EmbedTo_Stack, m_pActionPool, false /* show toolbar */);
                AssertPtrReturnVoid(m_pPaneMedia);
                {
                    /* Configure pane: */
                    m_pPaneMedia->setProperty("ToolType", QVariant::fromValue(UIToolType_Media));
                    connect(m_pPaneMedia, &UIMediumManagerWidget::sigCreateMedium,
                            this, &UIToolPane::sigCreateMedium);
                    connect(m_pPaneMedia, &UIMediumManagerWidget::sigCopyMedium,
                            this, &UIToolPane::sigCopyMedium);

                    /* Add into layout: */
                    addToolWidget(m_pPaneMedia);
                }
                break;
            }
            case UIToolType_Network:
            {
                /* Create Network Manager: */
                m_pPaneNetwork = new UINetworkManagerWidget(EmbedTo_Stack, m_pActionPool, false /* show toolbar */);
                AssertPtrReturnVoid(m_pPaneNetwork);
                {
                    /* Configure pane: */
                    m_pPaneNetwork->setProperty("ToolType", QVariant::fromValue(UIToolType_Network));

                    /* Add into layout: */
                    addToolWidget(m_pPaneNetwork);
                }
                break;
            }
            case UIToolType_Cloud:
            {
                /* Create Cloud Profile Manager: */
                m_pPaneCloud = new UICloudProfileManagerWidget(EmbedTo_Stack, m_pActionPool, false /* show toolbar */);
                AssertPtrReturnVoid(m_pPaneCloud);
                {
                    /* Configure pane: */
                    m_pPaneCloud->setProperty("ToolType", QVariant::fromValue(UIToolType_Cloud));

                    /* Add into layout: */
                    addToolWidget(m_pPaneCloud);
                }
                break;
            }
            case UIToolType_Resources:
            {
                /* Create VM Activity Overview: */
                m_pPaneVMActivityOverview = new UIVMActivityOverviewWidget(EmbedTo_Stack, m_pActionPool, false /* show toolbar */);
                AssertPtrReturnVoid(m_pPaneVMActivityOverview);
                {
                    /* Configure pane: */
                    m_pPaneVMActivityOverview->setProperty("ToolType", QVariant::fromValue(UIToolType_Resources));
                    connect(m_pPaneVMActivityOverview, &UIVMActivityOverviewWidget::sigSwitchToMachineActivityPane,
                            this, &UIToolPane::sigSwitchToMachineActivityPane);
                    m_pPaneVMActivityOverview->setCloudMachineItems(m_cloudItems);

                    /* Add into layout: */
                    addToolWidget(m_pPaneVMActivityOverview);
                }
                break;
            }
            case UIToolType_Error:
            {
                /* Create Error pane: */
                m_pPaneError = new UIErrorPane;
                AssertPtrReturnVoid(m_pPaneError);
                {
                    /* Configure pane: */
                    m_pPaneError->setProperty("ToolType", QVariant::fromValue(UIToolType_Error));
#ifndef VBOX_WS_MAC
                    const int iMargin = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 4;
                    m_pPaneError->setContentsMargins(iMargin, 0, iMargin, 0);
#endif

                    /* Add into layout: */
                    addToolWidget(m_pPaneError);
                }
                break;
            }
            case UIToolType_Details:
            {
                /* Create Details pane: */
                m_pPaneDetails = new UIDetails;
                AssertPtrReturnVoid(m_pPaneDetails);
                {
                    /* Configure pane: */
                    m_pPaneDetails->setProperty("ToolType", QVariant::fromValue(UIToolType_Details));
                    connect(this, &UIToolPane::sigToggleStarted,  m_pPaneDetails, &UIDetails::sigToggleStarted);
                    connect(this, &UIToolPane::sigToggleFinished, m_pPaneDetails, &UIDetails::sigToggleFinished);
                    connect(m_pPaneDetails, &UIDetails::sigLinkClicked,  this, &UIToolPane::sigLinkClicked);
                    m_pPaneDetails->setItems(m_items);

                    /* Add into layout: */
                    addToolWidget(m_pPaneDetails);
                }
                break;
            }
            case UIToolType_Snapshots:
            {
                /* Create Snapshots pane: */
                m_pPaneSnapshots = new UISnapshotPane(m_pActionPool, false /* show toolbar? */);
                AssertPtrReturnVoid(m_pPaneSnapshots);
                {
                    /* Configure pane: */
                    m_pPaneSnapshots->setProperty("ToolType", QVariant::fromValue(UIToolType_Snapshots));
                    connect(m_pPaneSnapshots, &UISnapshotPane::sigCurrentItemChange,
                            this, &UIToolPane::sigCurrentSnapshotItemChange);
                    m_pPaneSnapshots->setMachineItems(m_items);
#ifndef VBOX_WS_MAC
                    const int iMargin = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 4;
                    m_pPaneSnapshots->setContentsMargins(iMargin, 0, iMargin, 0);
#endif

                    /* Add into layout: */
                    addToolWidget(m_pPaneSnapshots);
                }
                break;
            }
            case UIToolType_Logs:
            {
                /* Create Logviewer pane: */
                m_pPaneLogViewer = new UIVMLogViewerWidget(EmbedTo_Stack, m_pActionPool, false /* show toolbar */);
                AssertPtrReturnVoid(m_pPaneLogViewer);
                {
                    /* Configure pane: */
                    m_pPaneLogViewer->setProperty("ToolType", QVariant::fromValue(UIToolType_Logs));
                    connect(m_pPaneLogViewer, &UIVMLogViewerWidget::sigDetach,
                            this, &UIToolPane::sltDetachToolPane);
                    m_pPaneLogViewer->setSelectedVMListItems(m_items);
#ifndef VBOX_WS_MAC
                    const int iMargin = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 4;
                    m_pPaneLogViewer->setContentsMargins(iMargin, 0, iMargin, 0);
#endif

                    /* Add into layout: */
                    addToolWidget(m_pPaneLogViewer);
                }
                break;
            }
            case UIToolType_ResourceUse:
            {
                /* Create VM Activity Tool pane: */
                m_pPaneVMActivityTool = new UIVMActivityToolWidget(EmbedTo_Stack, m_pActionPool,
                                                                   false /* Show toolbar */, 0 /* Parent */);
                AssertPtrReturnVoid(m_pPaneVMActivityTool);
                {
                    /* Configure pane: */
                    m_pPaneVMActivityTool->setProperty("ToolType", QVariant::fromValue(UIToolType_ResourceUse));
                    m_pPaneVMActivityTool->setSelectedVMListItems(m_items);
                    connect(m_pPaneVMActivityTool, &UIVMActivityToolWidget::sigSwitchToActivityOverviewPane,
                            this, &UIToolPane::sigSwitchToActivityOverviewPane);
#ifndef VBOX_WS_MAC
                    const int iMargin = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 4;
                    m_pPaneVMActivityTool->setContentsMargins(iMargin, 0, iMargin, 0);
#endif

                    /* Add into layout: */
                    addToolWidget(m_pPaneVMActivityTool);
                }
                break;
            }
            case UIToolType_FileManager:
            {
                /* Create File Manager pane: */
                if (!m_items.isEmpty())
                    m_pPaneFileManager = new UIFileManager(EmbedTo_Stack, m_pActionPool,
                                                           gpGlobalSession->virtualBox().FindMachine(m_items[0]->id().toString()),
                                                           0, false /* fShowToolbar */);
                else
                    m_pPaneFileManager = new UIFileManager(EmbedTo_Stack, m_pActionPool, CMachine(),
                                                           0, false /* fShowToolbar */);
                AssertPtrReturnVoid(m_pPaneFileManager);
                {
                    /* Configure pane: */
                    m_pPaneFileManager->setProperty("ToolType", QVariant::fromValue(UIToolType_FileManager));
                    m_pPaneFileManager->setSelectedVMListItems(m_items);
#ifndef VBOX_WS_MAC
                    const int iMargin = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 4;
                    m_pPaneFileManager->setContentsMargins(iMargin, 0, iMargin, 0);
#endif

                    /* Add into layout: */
                    addToolWidget(m_pPaneFileManager);
                }
                break;
            }
            default:
                AssertFailedReturnVoid();
        }
    }

    /* Handle token change: */
    handleTokenChange();
}

void UIToolPane::closeTool(UIToolType enmType)
{
    /* Search through the stacked widgets: */
    int iActualIndex = -1;
    for (int iIndex = 0; iIndex < m_pLayout->count(); ++iIndex)
        if (m_pLayout->widget(iIndex)->property("ToolType").value<UIToolType>() == enmType)
            iActualIndex = iIndex;

    /* If widget with such type doesn't exist: */
    if (iActualIndex != -1)
    {
        /* Forget corresponding widget: */
        switch (enmType)
        {
            case UIToolType_Home:        m_pPaneHome = 0; break;
            case UIToolType_Machines:    m_pPaneMachines = 0; break;
            case UIToolType_Phones:      m_pPanePhones = 0; break;
            case UIToolType_Extensions:  m_pPaneExtensions = 0; break;
            case UIToolType_Media:       m_pPaneMedia = 0; break;
            case UIToolType_Network:     m_pPaneNetwork = 0; break;
            case UIToolType_Cloud:       m_pPaneCloud = 0; break;
            case UIToolType_Resources:   m_pPaneVMActivityOverview = 0; break;
            case UIToolType_Error:       m_pPaneError = 0; break;
            case UIToolType_Details:     m_pPaneDetails = 0; break;
            case UIToolType_Snapshots:   m_pPaneSnapshots = 0; break;
            case UIToolType_Logs:        m_pPaneLogViewer = 0; break;
            case UIToolType_ResourceUse: m_pPaneVMActivityTool = 0; break;
            case UIToolType_FileManager: m_pPaneFileManager = 0; break;
            default: break;
        }
        /* Delete corresponding widget: */
        QWidget *pWidget = m_pLayout->widget(iActualIndex);
        m_pLayout->removeWidget(pWidget);
        delete pWidget;
    }

    /* Handle token change: */
    handleTokenChange();
}

QString UIToolPane::currentHelpKeyword() const
{
    QWidget *pCurrentToolWidget = 0;
    switch (currentTool())
    {
        case UIToolType_Home:
            pCurrentToolWidget = m_pPaneHome;
            break;
        case UIToolType_Machines:
            pCurrentToolWidget = m_pPaneMachines;
            break;
        case UIToolType_Phones:
            pCurrentToolWidget = m_pPanePhones;
            break;
        case UIToolType_Extensions:
            pCurrentToolWidget = m_pPaneExtensions;
            break;
        case UIToolType_Media:
            pCurrentToolWidget = m_pPaneMedia;
            break;
        case UIToolType_Network:
            pCurrentToolWidget = m_pPaneNetwork;
            break;
        case UIToolType_Cloud:
            pCurrentToolWidget = m_pPaneCloud;
            break;
        case UIToolType_Resources:
            pCurrentToolWidget = m_pPaneVMActivityOverview;
            break;
        case UIToolType_Error:
            pCurrentToolWidget = m_pPaneError;
            break;
        case UIToolType_Details:
            pCurrentToolWidget = m_pPaneDetails;
            break;
        case UIToolType_Snapshots:
            pCurrentToolWidget = m_pPaneSnapshots;
            break;
        case UIToolType_Logs:
            pCurrentToolWidget = m_pPaneLogViewer;
            break;
        case UIToolType_ResourceUse:
            pCurrentToolWidget = m_pPaneVMActivityTool;
            break;
        case UIToolType_FileManager:
            pCurrentToolWidget = m_pPaneFileManager;
            break;
        default:
            break;
    }
    return uiCommon().helpKeyword(pCurrentToolWidget);
}

void UIToolPane::setToolBar(QWidget *pToolBar)
{
    AssertReturnVoid(m_enmClass == UIToolClass_Global && !m_pToolBar);
    m_pToolBar = pToolBar;
    updateToolBarPlacement();
}

UIPhoneManager *UIToolPane::phoneManager()
{
    AssertReturn(m_enmClass == UIToolClass_Global, 0);
    if (!m_pPanePhones)
    {
        m_pPanePhones = new UIPhoneManager(this);
        m_pPanePhones->setProperty("ToolType", QVariant::fromValue(UIToolType_Phones));
        m_pLayout->addWidget(createToolCard(m_pPanePhones, UIToolType_Phones));
    }
    return m_pPanePhones;
}

UIMachineToolsWidget *UIToolPane::machineToolsWidget() const
{
    return m_pPaneMachines;
}

void UIToolPane::setCloudMachineItems(const QList<UIVirtualMachineItemCloud*> &cloudItems)
{
    /* Cache passed value: */
    m_cloudItems = cloudItems;

    /* Update VM Activity Overview pane if it is open: */
    if (isToolOpened(UIToolType_Resources))
    {
        AssertPtrReturnVoid(m_pPaneVMActivityOverview);
        m_pPaneVMActivityOverview->setCloudMachineItems(m_cloudItems);
    }
}

void UIToolPane::setErrorDetails(const QString &strDetails)
{
    /* Update Error pane: */
    if (m_pPaneError)
        m_pPaneError->setErrorDetails(strDetails);
}

void UIToolPane::setItems(const QList<UIVirtualMachineItem*> &items)
{
    /* Cache accessible items only.  Inaccessible machines expose limited
     * information and are handled by the Error pane. */
    m_items.clear();
    foreach (UIVirtualMachineItem *pItem, items)
        if (pItem && pItem->accessible())
            m_items << pItem;

    /* Update details pane if it is open: */
    if (isToolOpened(UIToolType_Details))
    {
        AssertPtrReturnVoid(m_pPaneDetails);
        m_pPaneDetails->setItems(m_items);
    }
    /* Update snapshots pane if it is open: */
    if (isToolOpened(UIToolType_Snapshots))
    {
        AssertPtrReturnVoid(m_pPaneSnapshots);
        m_pPaneSnapshots->setMachineItems(m_items);
    }
    /* Update logs pane if it is open: */
    if (isToolOpened(UIToolType_Logs))
    {
        AssertPtrReturnVoid(m_pPaneLogViewer);
        m_pPaneLogViewer->setSelectedVMListItems(m_items);
    }
    /* Update VM activity tool pane if it is open: */
    if (isToolOpened(UIToolType_ResourceUse))
    {
        AssertPtrReturnVoid(m_pPaneVMActivityTool);
        m_pPaneVMActivityTool->setSelectedVMListItems(m_items);
    }
    /* Update file manager pane if it is open: */
    if (isToolOpened(UIToolType_FileManager))
    {
        AssertPtrReturnVoid(m_pPaneFileManager);
        if (!m_items.isEmpty() && m_items[0])
            m_pPaneFileManager->setSelectedVMListItems(m_items);
    }
}

bool UIToolPane::isCurrentStateItemSelected() const
{
    return m_pPaneSnapshots ? m_pPaneSnapshots->isCurrentStateItemSelected() : false;
}

QUuid UIToolPane::currentSnapshotId()
{
    return m_pPaneSnapshots ? m_pPaneSnapshots->currentSnapshotId() : QUuid();
}

void UIToolPane::sltDetachToolPane()
{
    AssertPtrReturnVoid(sender());
    UIToolType enmToolType = UIToolType_Invalid;
    if (sender() == m_pPaneLogViewer)
        enmToolType = UIToolType_Logs;
    if (enmToolType != UIToolType_Invalid)
        emit sigDetachToolPane(enmToolType);
}

void UIToolPane::prepare()
{
    /* Create stacked-layout: */
    m_pLayout = new QStackedLayout;
    m_pLayout->setContentsMargins(0, 0, 0, 0);
    if (m_enmClass == UIToolClass_Global)
    {
        QVBoxLayout *pOuterLayout = new QVBoxLayout(this);
        pOuterLayout->setContentsMargins(0, 0, 0, 0);
        pOuterLayout->setSpacing(0);
        pOuterLayout->addLayout(m_pLayout);
        setStyleSheet(UIManagerStyle::toolPageStyleSheet(QApplication::palette()));
        connect(m_pLayout, &QStackedLayout::currentChanged, this, [this, pOuterLayout]
        {
            /* Home applies the same gutter inside its scroll area. */
            const bool fCard = currentTool() != UIToolType_Home && currentTool() != UIToolType_Invalid;
            const int iInset = fCard ? UIManagerStyle::pageInset() : 0;
            pOuterLayout->setContentsMargins(iInset, fCard ? 12 : 0, iInset, iInset);
            updateToolBarPlacement();
        });
    }
    else
        setLayout(m_pLayout);

    /* Open default tools: */
    switch (m_enmClass)
    {
        case UIToolClass_Global:
        {
            /* Create welcome pane: */
            openTool(UIToolType_Home);
            /* Create machines pane: */
            openTool(UIToolType_Machines);

            break;
        }
        case UIToolClass_Machine:
        {
            /* Create Details pane: */
            openTool(UIToolType_Details);

            break;
        }
        default:
            break;
    }
}

void UIToolPane::addToolWidget(QWidget *pWidget)
{
    const UIToolType enmType = pWidget->property("ToolType").value<UIToolType>();
    if (m_enmClass == UIToolClass_Global && enmType != UIToolType_Home)
        pWidget = createToolCard(pWidget, enmType);
    m_pLayout->addWidget(pWidget);
    m_pLayout->setCurrentWidget(pWidget);
}

/** Draws one line of header text while retaining the full accessible label. */
class UIToolHeaderLabel : public QLabel
{
public:
    /** Constructs the label with the given @a pParent. */
    explicit UIToolHeaderLabel(QWidget *pParent)
        : QLabel(pParent)
    {
        setTextFormat(Qt::PlainText);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }

    /** Allows the toolbar and the text to share a narrow header. */
    virtual QSize minimumSizeHint() const RT_OVERRIDE
    {
        const QSize size = sizeHint();
        return QSize(qMin(size.width(), 12 * fontMetrics().averageCharWidth()), size.height());
    }

protected:
    /** Draws the label with an ellipsis if the text exceeds the available width. */
    virtual void paintEvent(QPaintEvent *) RT_OVERRIDE
    {
        QStylePainter painter(this);
        painter.setFont(font());
        const QRect bounds = contentsRect();
        const QString strText = fontMetrics().elidedText(text(), Qt::ElideRight, bounds.width());
        painter.drawItemText(bounds, QStyle::visualAlignment(layoutDirection(), alignment()),
                             palette(), isEnabled(), strText, foregroundRole());
    }
};

/** Returns the localized description of a Manager tool. */
static QString uiToolPaneDescription(UIToolType enmType)
{
    switch (enmType)
    {
        case UIToolType_Machines:   return UIToolPane::tr("Manage virtual machines, snapshots and settings.");
        case UIToolType_Phones:     return UIToolPane::tr("Create and manage your Android virtual devices.");
        case UIToolType_Extensions: return UIToolPane::tr("View and manage installed extension packs.");
        case UIToolType_Media:      return UIToolPane::tr("Manage virtual hard disks, optical images and floppy disks.");
        case UIToolType_Network:    return UIToolPane::tr("Manage host-only, NAT and cloud networks.");
        case UIToolType_Cloud:      return UIToolPane::tr("Manage profiles that connect to your cloud providers.");
        case UIToolType_Resources:  return UIToolPane::tr("Monitor host resources and virtual machine activity.");
        default:                   return QString();
    }
}

QWidget *UIToolPane::createToolCard(QWidget *pTool, UIToolType enmType)
{
    QFrame *pCard = new QFrame(this);
    pCard->setObjectName(QStringLiteral("managerToolCard"));
    pCard->setProperty("ToolType", QVariant::fromValue(enmType));
    QVBoxLayout *pLayout = new QVBoxLayout(pCard);
    pLayout->setContentsMargins(20, 10, 20, 20);
    pLayout->setSpacing(8);

    QWidget *pHeader = new QWidget(pCard);
    pHeader->setObjectName(QStringLiteral("managerPageHeader"));
    QHBoxLayout *pHeading = new QHBoxLayout(pHeader);
    pHeading->setContentsMargins(0, 0, 0, 0);
    pHeading->setSpacing(12);
    QLabel *pIcon = new QLabel(pHeader);
    pIcon->setObjectName(QStringLiteral("managerPageIcon"));
    pIcon->setFixedSize(42, 42);
    pIcon->setAlignment(Qt::AlignCenter);
    pIcon->setPixmap(UIToolsItem::typeToIcon(enmType).pixmap(QSize(24, 24), pCard->devicePixelRatioF()));
    pHeading->addWidget(pIcon, 0, Qt::AlignVCenter);

    QWidget *pText = new QWidget(pHeader);
    QLabel *pTitle = new UIToolHeaderLabel(pText);
    pTitle->setObjectName(QStringLiteral("managerPageTitle"));
    QLabel *pDescription = new UIToolHeaderLabel(pText);
    pDescription->setObjectName(QStringLiteral("managerPageDescription"));
    QVBoxLayout *pLabels = new QVBoxLayout(pText);
    pLabels->setContentsMargins(0, 0, 0, 0);
    pLabels->setSpacing(0);
    pLabels->addWidget(pTitle);
    pLabels->addWidget(pDescription);
    pHeading->addWidget(pText, 1, Qt::AlignVCenter);
    pLayout->addWidget(pHeader);

    const auto retranslateHeader = [pTitle, pDescription, enmType]
    {
        pTitle->setText(UIToolsItem::typeToName(enmType));
        pTitle->setToolTip(pTitle->text());
        pDescription->setText(uiToolPaneDescription(enmType));
        pDescription->setToolTip(pDescription->text());
    };
    connect(&translationEventListener(), &UITranslationEventListener::sigRetranslateUI, pCard, retranslateHeader);
    retranslateHeader();

    QFrame *pDivider = new QFrame(pCard);
    pDivider->setObjectName(QStringLiteral("managerPageDivider"));
    pDivider->setFixedHeight(1);
    pLayout->addWidget(pDivider);
    pLayout->addWidget(pTool, 1);
    return pCard;
}

void UIToolPane::updateToolBarPlacement()
{
    if (!m_pToolBar)
        return;

    QWidget *pPage = m_pLayout->currentWidget();
    QWidget *pHeader = pPage ? pPage->findChild<QWidget*>(QStringLiteral("managerPageHeader"),
                                                          Qt::FindDirectChildrenOnly) : 0;
    if (pHeader)
    {
        if (pHeader->layout()->indexOf(m_pToolBar) < 0)
            pHeader->layout()->addWidget(m_pToolBar);
        m_pToolBar->show();
    }
    else
    {
        /* Park it outside the stack so removing the old page cannot delete it. */
        m_pToolBar->hide();
        m_pToolBar->setParent(this);
    }
}

void UIToolPane::changeEvent(QEvent *pEvent)
{
    QWidget::changeEvent(pEvent);
    if (   m_enmClass == UIToolClass_Global
        && (pEvent->type() == QEvent::PaletteChange || pEvent->type() == QEvent::ApplicationPaletteChange))
    {
        const QString strStyle = UIManagerStyle::toolPageStyleSheet(QApplication::palette());
        if (styleSheet() != strStyle)
            setStyleSheet(strStyle);
    }
}

void UIToolPane::cleanup()
{
    /* Remove all widgets prematurelly: */
    while (m_pLayout->count())
    {
        QWidget *pWidget = m_pLayout->widget(0);
        m_pLayout->removeWidget(pWidget);
        delete pWidget;
    }
}

void UIToolPane::handleTokenChange()
{
    /* Determine whether VM Activity Overview pane should be currently active tool: */
    if (m_pPaneVMActivityOverview)
        m_pPaneVMActivityOverview->setIsCurrentTool(m_fActive && currentTool() == UIToolType_Resources);
}
