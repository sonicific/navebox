/* $Id: UITools.cpp 112403 2026-01-11 19:29:08Z knut.osmundsen@oracle.com $ */
/** @file
 * VBox Qt GUI - UITools class implementation.
 */

/*
 * Copyright (C) 2012-2026 Oracle and/or its affiliates.
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
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIBranding.h"
#include "UICommon.h"
#include "UIExtraDataManager.h"
#include "UIManagerStyle.h"
#include "UITools.h"
#include "UIToolsModel.h"
#include "UIToolsView.h"
#include "UITranslationEventListener.h"
#include "UIVirtualBoxManager.h"

/* Other VBox includes: */
#include "iprt/assert.h"


UITools::UITools(QWidget *pParent, UIToolClass enmClass)
    : QWidget(pParent, Qt::Widget)
    , m_enmClass(enmClass)
    , m_enmAlignment(m_enmClass == UIToolClass_Machine ? Qt::Horizontal : Qt::Vertical)
    , m_pMainLayout(0)
    , m_pToolsModel(0)
    , m_pToolsView(0)
{
    prepare();
}

UITools::~UITools()
{
    cleanup();
}

void UITools::setToolsType(UIToolType enmType)
{
    m_pToolsModel->setToolsType(enmType);
}

UIToolType UITools::toolsType() const
{
    return m_pToolsModel->toolsType();
}

void UITools::setItemsEnabled(bool fEnabled)
{
    m_pToolsModel->setItemsEnabled(fEnabled);
}

bool UITools::isItemsEnabled() const
{
    return m_pToolsModel->isItemsEnabled();
}

void UITools::setRestrictedToolTypes(const QList<UIToolType> &types)
{
    m_pToolsModel->setRestrictedToolTypes(types);
}

void UITools::prepare()
{
    /* Setup own layout rules: */
    switch (m_enmAlignment)
    {
        case Qt::Vertical:
            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
            break;
        case Qt::Horizontal:
            setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            break;
    }

    /* Prepare model: */
    m_pToolsModel = new UIToolsModel(this, m_enmClass);
    if (m_pToolsModel)
    {
        /* Prepare main-layout: */
        m_pMainLayout = new QVBoxLayout(this);
        if (m_pMainLayout)
        {
            m_pMainLayout->setContentsMargins(1, 1, 1, 1);
            m_pMainLayout->setSpacing(0);

            /* Prepare view: */
            m_pToolsView = new UIToolsView(this, m_enmClass, m_pToolsModel);
            if (m_pToolsView)
            {
                m_pToolsView->show();
                setFocusProxy(m_pToolsView);

                /* Add into layout: */
                m_pMainLayout->addWidget(m_pToolsView);
            }
        }

        /* Prepare model connections: */
        connect(m_pToolsModel, &UIToolsModel::sigSelectionChanged,
                this, &UITools::sigSelectionChanged);

        if (m_enmClass == UIToolClass_Global)
            prepareNavigation();

        /* Init model finally: */
        m_pToolsModel->init();
    }
}

void UITools::prepareNavigation()
{
    QWidget *pBrandHeader = new QWidget(this);
    pBrandHeader->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QHBoxLayout *pBrand = new QHBoxLayout(pBrandHeader);
    pBrand->setContentsMargins(8, 10, 8, 9);
    pBrand->setSpacing(7);
    m_pBrandIcon = new QLabel(pBrandHeader);
    m_pBrandIcon->setFixedSize(24, 24);
    m_pBrandIcon->setAlignment(Qt::AlignCenter);
    m_pBrandIcon->setToolTip(UIBranding::name());
    pBrand->addWidget(m_pBrandIcon, 0, Qt::AlignHCenter);
    m_pBrandName = new QLabel(UIBranding::name(), pBrandHeader);
    QFont font = m_pBrandName->font();
    font.setBold(true);
    if (font.pointSizeF() > 0)
        font.setPointSizeF(font.pointSizeF() * 1.25);
    else
        font.setPixelSize(qRound(font.pixelSize() * 1.25));
    m_pBrandName->setFont(font);
    pBrand->addWidget(m_pBrandName, 1);
    m_pMainLayout->insertWidget(0, pBrandHeader);

    QHBoxLayout *pFooter = new QHBoxLayout;
    pFooter->setContentsMargins(6, 5, 6, 6);
    m_pNavigationToggle = new QToolButton(this);
    m_pNavigationToggle->setObjectName(QStringLiteral("managerNavigationToggle"));
    m_pNavigationToggle->setIconSize(QSize(18, 18));
    m_pNavigationToggle->setFocusPolicy(Qt::TabFocus);
    m_pNavigationToggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    pFooter->addWidget(m_pNavigationToggle);
    m_pMainLayout->addLayout(pFooter);

    connect(m_pNavigationToggle, &QToolButton::clicked, this, []
    {
        gEDataManager->setToolTextVisible(!gEDataManager->isToolTextVisible());
    });
    connect(gEDataManager, &UIExtraDataManager::sigToolLabelsVisibilityChange, this, &UITools::sltUpdateNavigation);
    connect(&uiCommon(), &UICommon::sigThemeChange, this, &UITools::sltUpdateNavigation);
    connect(&translationEventListener(), &UITranslationEventListener::sigRetranslateUI, this, &UITools::sltUpdateNavigation);
    connect(gpManager, &UIVirtualBoxManager::sigWindowRemapped, this, &UITools::sltUpdateNavigation);
    connect(qApp, &QGuiApplication::layoutDirectionChanged, this, &UITools::sltUpdateNavigation);
    sltUpdateNavigation();
}

void UITools::sltUpdateNavigation()
{
    const bool fExpanded = gEDataManager->isToolTextVisible();
    const QString strAction = fExpanded ? tr("Collapsed") : tr("Expand");
    m_pBrandName->setVisible(fExpanded);
    m_pBrandIcon->setPixmap(UIBranding::icon().pixmap(m_pBrandIcon->size(), devicePixelRatioF()));
    m_pNavigationToggle->setText(strAction);
    m_pNavigationToggle->setToolTip(strAction);
    m_pNavigationToggle->setAccessibleName(strAction);
    m_pNavigationToggle->setToolButtonStyle(fExpanded ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonTextUnderIcon);
    m_pNavigationToggle->setIcon(UIManagerStyle::navigationToggleIcon(QApplication::palette(),
                                     fExpanded, layoutDirection(), devicePixelRatioF()));
    const QString strStyle = UIManagerStyle::navigationToggleStyleSheet(QApplication::palette(), fExpanded);
    if (m_pNavigationToggle->styleSheet() != strStyle)
        m_pNavigationToggle->setStyleSheet(strStyle);
}

void UITools::cleanup()
{
    /* Cleanup model connections: */
    if (m_pToolsModel)
        disconnect(m_pToolsModel, &UIToolsModel::sigSelectionChanged,
                   this, &UITools::sigSelectionChanged);

    /* Cleanup view: */
    delete m_pToolsView;
    m_pToolsView = 0;

    /* Cleanup model: */
    delete m_pToolsModel;
    m_pToolsModel = 0;
}
