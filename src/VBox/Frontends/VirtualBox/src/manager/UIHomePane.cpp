/* $Id: UIHomePane.cpp 112403 2026-01-11 19:29:08Z knut.osmundsen@oracle.com $ */
/** @file
 * VBox Qt GUI - UIHomePane class implementation.
 */

/*
 * Copyright (C) 2010-2026 Oracle and/or its affiliates.
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
#include <QImageReader>
#include <QKeySequence>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIBranding.h"
#include "UICommon.h"
#include "UIExtraDataManager.h"
#include "UIHomePane.h"
#include "UIIconPool.h"
#include "UIManagerStyle.h"
#include "UITranslationEventListener.h"

/** @page pg_ui_home Home Pane
 *
 * Presents shortcuts to the manager's existing actions and help resources.
 * The first-run experience mode chooser sits below the shortcuts.
 */

namespace
{
    /** Minimum pane width for showing the banner illustration beside the title. */
    constexpr int g_iHomeArtworkMinimumWidth = 700;

    /** Description and destination for one shortcut. */
    struct HomeAction
    {
        const char *pszIcon;               /**< Resource for the row icon. */
        const char *pszTitle;              /**< Translatable action title. */
        const char *pszDescription;        /**< Translatable action description. */
        HomeTask    enmTask;               /**< Manager task, if the action is local. */
        const char *pszUrl;                /**< Browser destination for external actions. */
        bool        fHelpShortcut = false; /**< Whether the description includes the help shortcut. */
    };

    /** Quick actions in visual and keyboard navigation order. */
    const HomeAction g_aHomeActions[] =
    {
        { ":/global_settings_32px.png", QT_TRANSLATE_NOOP("UIHomePane", "Configure %1"),
          QT_TRANSLATE_NOOP("UIHomePane", "Open global preferences"), HomeTask_Configure, 0 },
        { ":/vm_new_32px.png", QT_TRANSLATE_NOOP("UIHomePane", "Create a new virtual machine"),
          QT_TRANSLATE_NOOP("UIHomePane", "Set up a VM for a new operating system"), HomeTask_Create, 0 },
        { ":/vm_add_32px.png", QT_TRANSLATE_NOOP("UIHomePane", "Open a saved virtual machine"),
          QT_TRANSLATE_NOOP("UIHomePane", "Add an existing virtual machine from disk"), HomeTask_Open, 0 },
        { ":/import_32px.png", QT_TRANSLATE_NOOP("UIHomePane", "Import a virtual machine"),
          QT_TRANSLATE_NOOP("UIHomePane", "Bring in a VM from an appliance file or the cloud"), HomeTask_Import, 0 },
        { ":/export_32px.png", QT_TRANSLATE_NOOP("UIHomePane", "Export a virtual machine"),
          QT_TRANSLATE_NOOP("UIHomePane", "Save one or more virtual machines as an appliance"), HomeTask_Export, 0 },
        { ":/navebox_phone_32px.png", QT_TRANSLATE_NOOP("UIHomePane", "Open Android Phones"),
          QT_TRANSLATE_NOOP("UIHomePane", "Create and manage Android virtual phones"), HomeTask_Phones, 0 },
        { ":/help_16px.png", QT_TRANSLATE_NOOP("UIHomePane", "Read the VirtualBox documentation"),
          QT_TRANSLATE_NOOP("UIHomePane", "Browse the online manual or press %1 for help"), HomeTask_Invalid,
          "https://docs.oracle.com/en/virtualization/virtualbox/index.html", true },
        { ":/site_32px.png", QT_TRANSLATE_NOOP("UIHomePane", "Visit the VirtualBox community"),
          QT_TRANSLATE_NOOP("UIHomePane", "Find community resources, test builds and source code"), HomeTask_Invalid,
          "https://www.virtualbox.org" },
    };
}


/** A keyboard-accessible shortcut with an icon, title and description. */
class UIHomeActionButton : public QPushButton
{
public:

    /** Constructs a shortcut with @a icon and the given @a pParent. */
    UIHomeActionButton(const QIcon &icon, QWidget *pParent)
        : QPushButton(pParent)
        , m_icon(icon)
        , m_pIcon(new QLabel(this))
        , m_pTitle(new QLabel(this))
        , m_pDescription(new QLabel(this))
    {
        setObjectName(QStringLiteral("homeAction"));
        setAutoDefault(false);
        setFocusPolicy(Qt::TabFocus);
        setCursor(Qt::PointingHandCursor);
        setMinimumHeight(54);
        QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        policy.setHeightForWidth(true);
        setSizePolicy(policy);

        m_pIcon->setFixedSize(28, 28);
        m_pIcon->setAlignment(Qt::AlignCenter);
        m_pIcon->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_pTitle->setObjectName(QStringLiteral("homePrimaryText"));
        m_pTitle->setTextFormat(Qt::PlainText);
        m_pTitle->setWordWrap(true);
        m_pTitle->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_pDescription->setObjectName(QStringLiteral("homeSecondaryText"));
        m_pDescription->setTextFormat(Qt::PlainText);
        m_pDescription->setWordWrap(true);
        m_pDescription->setAttribute(Qt::WA_TransparentForMouseEvents);

        QFont fontTitle = m_pTitle->font();
        fontTitle.setWeight(QFont::DemiBold);
        m_pTitle->setFont(fontTitle);

        QVBoxLayout *pTextLayout = new QVBoxLayout;
        pTextLayout->setSpacing(2);
        pTextLayout->addWidget(m_pTitle);
        pTextLayout->addWidget(m_pDescription);

        QHBoxLayout *pLayout = new QHBoxLayout(this);
        pLayout->setContentsMargins(14, 8, 14, 8);
        pLayout->setSpacing(14);
        pLayout->addWidget(m_pIcon);
        pLayout->addLayout(pTextLayout, 1);
        pLayout->addSpacing(18);
    }

    /** Sets both visible and accessible text. */
    void setActionText(const QString &strTitle, const QString &strDescription)
    {
        m_pTitle->setText(strTitle);
        m_pDescription->setText(strDescription);
        setAccessibleName(strTitle);
        setAccessibleDescription(strDescription);
    }

    /** Refreshes the icon for @a fDevicePixelRatio. */
    void updateIcon(qreal fDevicePixelRatio)
    {
        m_pIcon->setPixmap(m_icon.pixmap(m_pIcon->size(), fDevicePixelRatio));
    }

    /** Returns the preferred row size, including the text and icon. */
    virtual QSize sizeHint() const RT_OVERRIDE { return layout()->totalSizeHint(); }

    /** Returns the space needed for the row to wrap its text. */
    virtual QSize minimumSizeHint() const RT_OVERRIDE { return layout()->totalMinimumSize(); }

protected:

    /** Paints the button and the trailing chevron. */
    virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE
    {
        QPushButton::paintEvent(pEvent);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QColor colorArrow = m_pDescription->palette().color(m_pDescription->foregroundRole());
        colorArrow.setAlpha(200);
        painter.setPen(QPen(colorArrow, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        const bool fRightToLeft = isRightToLeft();
        painter.translate(fRightToLeft ? 22 : width() - 22, height() / 2.0);
        if (fRightToLeft)
            painter.scale(-1, 1);
        const QPoint aPoints[] = { QPoint(-2, -4), QPoint(2, 0), QPoint(-2, 4) };
        painter.drawPolyline(aPoints, 3);
    }

private:

    QIcon   m_icon;         /**< Holds the action icon. */
    QLabel *m_pIcon;        /**< Holds the icon label. */
    QLabel *m_pTitle;       /**< Holds the action title. */
    QLabel *m_pDescription; /**< Holds the action description. */
};


/*********************************************************************************************************************************
*   Class UIHomePane implementation.                                                                                             *
*********************************************************************************************************************************/

UIHomePane::UIHomePane(QWidget *pParent /* = 0 */)
    : QWidget(pParent)
    , m_pCard(0)
    , m_pLabelIcon(0)
    , m_pLabelTitle(0)
    , m_pLabelSubtitle(0)
    , m_pLabelArtwork(0)
    , m_iconBrand(UIBranding::icon())
    , m_pExperienceMode(0)
    , m_pLabelMode(0)
    , m_pButtonBasic(0)
    , m_pButtonExpert(0)
{
    prepare();
}

bool UIHomePane::event(QEvent *pEvent)
{
    const bool fResult = QWidget::event(pEvent);
    switch (pEvent->type())
    {
        case QEvent::PaletteChange:
        case QEvent::ApplicationPaletteChange:
            if (m_pCard)
                updateAppearance();
            break;
        case QEvent::Show:
        case QEvent::ScreenChangeInternal:
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        case QEvent::DevicePixelRatioChange:
#endif
            if (m_pLabelArtwork)
                updatePixmaps();
            break;
        default:
            break;
    }
    return fResult;
}

void UIHomePane::resizeEvent(QResizeEvent *pEvent)
{
    QWidget::resizeEvent(pEvent);
    m_pLabelArtwork->setVisible(pEvent->size().width() >= g_iHomeArtworkMinimumWidth
                                && !m_pLabelArtwork->pixmap().isNull());
}

void UIHomePane::sltRetranslateUI()
{
    m_pLabelTitle->setText(tr("Welcome to %1").arg(UIBranding::name()));
    m_pLabelSubtitle->setText(tr("Quick actions and resources for your virtual machines"));

    for (qsizetype i = 0; i < m_actionButtons.size(); ++i)
    {
        const HomeAction &action = g_aHomeActions[i];
        QString strTitle = tr(action.pszTitle);
        QString strDescription = tr(action.pszDescription);
        if (action.enmTask == HomeTask_Configure)
            strTitle = strTitle.arg(UIBranding::name());
        if (action.fHelpShortcut)
            strDescription = strDescription.arg(QKeySequence(QKeySequence::HelpContents).toString(QKeySequence::NativeText));
        m_actionButtons.at(i)->setActionText(strTitle, strDescription);
    }

    if (m_pExperienceMode)
    {
        m_pLabelMode->setText(tr("<h3>Please choose Experience Mode!</h3>"
                                 "By default, %1 is hiding some options, tools and wizards. "
                                 "<p>The <b>Basic Mode</b> is intended for those users who are not interested in advanced "
                                 "functionality and prefer a simpler, cleaner interface.</p>"
                                 "<p>The <b>Expert Mode</b> is intended for experienced users who wish to utilize all "
                                 "%1 functionality.</p>"
                                 "<p>You can choose whether you are a beginner or experienced user by selecting required "
                                 "option at the right. This choice can always be changed in Global Preferences or Machine "
                                 "Settings windows.</p>").arg(UIBranding::name().toHtmlEscaped()));
        m_pButtonBasic->setText(tr("Basic Mode"));
        m_pButtonExpert->setText(tr("Expert Mode"));
    }
}

void UIHomePane::prepare()
{
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    pMainLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *pScrollArea = new QScrollArea(this);
    pScrollArea->setFrameShape(QFrame::NoFrame);
    pScrollArea->setWidgetResizable(true);
    pMainLayout->addWidget(pScrollArea);

    QWidget *pContents = new QWidget;
    pContents->setObjectName(QStringLiteral("homeContents"));
    pContents->setAttribute(Qt::WA_StyledBackground);
    QVBoxLayout *pContentsLayout = new QVBoxLayout(pContents);
    const int iInset = UIManagerStyle::pageInset();
    pContentsLayout->setContentsMargins(iInset, 12, iInset, 14);
    pScrollArea->setWidget(pContents);
    pScrollArea->viewport()->setAutoFillBackground(false);
    pContents->setAutoFillBackground(false);

    m_pCard = new QFrame(pContents);
    m_pCard->setObjectName(QStringLiteral("homeCard"));
    pContentsLayout->addWidget(m_pCard);
    pContentsLayout->addStretch();

    QVBoxLayout *pCardLayout = new QVBoxLayout(m_pCard);
    pCardLayout->setContentsMargins(20, 14, 20, 20);
    pCardLayout->setSpacing(8);

    prepareHeader(pCardLayout);
    prepareActions(pCardLayout);
    if (gEDataManager->extraDataString(UIExtraDataDefs::GUI_Settings_ExpertMode).isNull())
        prepareExperienceMode(pCardLayout);

    UICommon::setHelpKeyword(this, "ct_about-virtualbox" /* help keyword */);
    connect(&translationEventListener(), &UITranslationEventListener::sigRetranslateUI,
            this, &UIHomePane::sltRetranslateUI);

    sltRetranslateUI();
    updateAppearance();
    updatePixmaps();
}

void UIHomePane::prepareHeader(QVBoxLayout *pLayout)
{
    QHBoxLayout *pHeaderLayout = new QHBoxLayout;
    pHeaderLayout->setSpacing(16);
    pLayout->addLayout(pHeaderLayout);

    m_pLabelIcon = new QLabel(m_pCard);
    m_pLabelIcon->setFixedSize(64, 64);
    m_pLabelIcon->setAlignment(Qt::AlignCenter);
    pHeaderLayout->addWidget(m_pLabelIcon);

    m_pLabelTitle = new QLabel(m_pCard);
    m_pLabelTitle->setObjectName(QStringLiteral("homePrimaryText"));
    m_pLabelTitle->setTextFormat(Qt::PlainText);
    m_pLabelTitle->setWordWrap(true);
    QFont fontTitle = m_pLabelTitle->font();
    fontTitle.setBold(true);
    if (fontTitle.pointSizeF() > 0)
        fontTitle.setPointSizeF(fontTitle.pointSizeF() * 1.9);
    else
        fontTitle.setPixelSize(qRound(fontTitle.pixelSize() * 1.9));
    m_pLabelTitle->setFont(fontTitle);

    m_pLabelSubtitle = new QLabel(m_pCard);
    m_pLabelSubtitle->setObjectName(QStringLiteral("homeSecondaryText"));
    m_pLabelSubtitle->setTextFormat(Qt::PlainText);
    m_pLabelSubtitle->setWordWrap(true);

    QVBoxLayout *pTextLayout = new QVBoxLayout;
    pTextLayout->setSpacing(4);
    pTextLayout->addStretch();
    pTextLayout->addWidget(m_pLabelTitle);
    pTextLayout->addWidget(m_pLabelSubtitle);
    pTextLayout->addStretch();
    pHeaderLayout->addLayout(pTextLayout, 1);

    m_pLabelArtwork = new QLabel(m_pCard);
    m_pLabelArtwork->setFixedSize(280, 91);
    m_pLabelArtwork->setAlignment(Qt::AlignCenter);
    m_pLabelArtwork->hide();
    pHeaderLayout->addWidget(m_pLabelArtwork);

    QFrame *pDivider = new QFrame(m_pCard);
    pDivider->setObjectName(QStringLiteral("homeDivider"));
    pDivider->setFixedHeight(1);
    pLayout->addWidget(pDivider);
    pLayout->addSpacing(4);
}

void UIHomePane::prepareActions(QVBoxLayout *pLayout)
{
    for (const HomeAction &action : g_aHomeActions)
    {
        UIHomeActionButton *pButton = new UIHomeActionButton(UIIconPool::iconSet(action.pszIcon), m_pCard);
        if (action.pszUrl)
        {
            const QString strUrl = QString::fromLatin1(action.pszUrl);
            connect(pButton, &QPushButton::clicked, this, [strUrl] { uiCommon().openURL(strUrl); });
        }
        else
        {
            const HomeTask enmTask = action.enmTask;
            connect(pButton, &QPushButton::clicked, this, [this, enmTask] { emit sigHomeTask(enmTask); });
        }
        pLayout->addWidget(pButton);
        m_actionButtons.append(pButton);
    }
}

void UIHomePane::prepareExperienceMode(QVBoxLayout *pLayout)
{
    m_pExperienceMode = new QWidget(m_pCard);
    m_pExperienceMode->setProperty("managerActionBar", true);
    QVBoxLayout *pModeLayout = new QVBoxLayout(m_pExperienceMode);
    pModeLayout->setContentsMargins(0, 8, 0, 0);
    pLayout->addWidget(m_pExperienceMode);

    m_pLabelMode = new QLabel(m_pExperienceMode);
    m_pLabelMode->setTextFormat(Qt::RichText);
    m_pLabelMode->setWordWrap(true);
    pModeLayout->addWidget(m_pLabelMode);

    QHBoxLayout *pButtonsLayout = new QHBoxLayout;
    pButtonsLayout->addStretch();
    pModeLayout->addLayout(pButtonsLayout);

    m_pButtonBasic = new QPushButton(m_pExperienceMode);
    m_pButtonExpert = new QPushButton(m_pExperienceMode);
    pButtonsLayout->addWidget(m_pButtonBasic);
    pButtonsLayout->addWidget(m_pButtonExpert);
    connect(m_pButtonBasic, &QPushButton::clicked, this, [this] { chooseExperienceMode(false); });
    connect(m_pButtonExpert, &QPushButton::clicked, this, [this] { chooseExperienceMode(true); });
}

void UIHomePane::chooseExperienceMode(bool fExpertMode)
{
    if (!gEDataManager->setSettingsInExpertMode(fExpertMode, this))
        return;

    m_pExperienceMode->hide();
    m_actionButtons.first()->setFocus(Qt::OtherFocusReason);
}

void UIHomePane::updateAppearance()
{
    QWidget *pContents = m_pCard->parentWidget();
    const QString strStyle = UIManagerStyle::homePaneStyleSheet(QApplication::palette());
    if (pContents->styleSheet() != strStyle)
        pContents->setStyleSheet(strStyle);
}

void UIHomePane::updatePixmaps()
{
    const qreal fDevicePixelRatio = devicePixelRatioF();
    m_pLabelIcon->setPixmap(m_iconBrand.pixmap(m_pLabelIcon->size(), fDevicePixelRatio));

    /* Use the SVG image plugin shipped with the application to rasterize at the display scale. */
    QImageReader reader(QStringLiteral(":/navebox-home-banner.svg"));
    reader.setScaledSize(m_pLabelArtwork->size() * fDevicePixelRatio);
    QPixmap artwork = QPixmap::fromImage(reader.read());
    artwork.setDevicePixelRatio(fDevicePixelRatio);
    m_pLabelArtwork->setPixmap(artwork);
    m_pLabelArtwork->setVisible(width() >= g_iHomeArtworkMinimumWidth && !artwork.isNull());

    for (UIHomeActionButton *pButton : m_actionButtons)
        pButton->updateIcon(fDevicePixelRatio);
}
