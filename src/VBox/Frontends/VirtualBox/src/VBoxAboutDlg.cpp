/* $Id: VBoxAboutDlg.cpp 113629 2026-03-27 14:38:08Z sergey.dubov@oracle.com $ */
/** @file
 * VBox Qt GUI - VBoxAboutDlg class implementation.
 */

/*
 * Copyright (C) 2006-2026 Oracle and/or its affiliates.
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
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIDialogButtonBox.h"
#include "UIBranding.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UITranslationEventListener.h"
#include "VBoxAboutDlg.h"

/* Other VBox includes: */
#include <VBox/version.h>

/** @page pg_ui_navebox_about Navebox Product Information
 *
 * Present the product identity and the four NAVE pillars separately from
 * contact details, build information and upstream attribution.  Content can
 * scroll on small screens; font metrics and layouts handle display scaling.
 */

/** Adds a word-wrapped label with the requested type hierarchy. */
static QLabel *uiAboutLabel(const QString &strText, QWidget *pParent, int iPointDelta = 0, bool fBold = false)
{
    QLabel *pLabel = new QLabel(strText, pParent);
    pLabel->setWordWrap(true);
    pLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
    QFont labelFont = pLabel->font();
    labelFont.setPointSize(qMax(8, labelFont.pointSize() + iPointDelta));
    labelFont.setBold(fBold);
    pLabel->setFont(labelFont);
    return pLabel;
}

/** Returns a link color with enough visual weight on a dark About surface. */
static QString uiAboutLinkColor(const QPalette &palette)
{
    QColor link = palette.color(QPalette::Link);
    if (palette.color(QPalette::Window).lightness() < 128 && link.lightness() < 160)
        link = link.lighter(180);
    return link.name();
}

VBoxAboutDlg::VBoxAboutDlg(QWidget *pParent /* = 0 */, const QString &strVersion /* = QString() */)
#ifdef VBOX_WS_MAC
    : QDialog(0)
    , m_pPseudoParent(pParent)
#else
    : QDialog(pParent)
    , m_pPseudoParent(0)
#endif
    , m_fPolished(false)
    , m_strVersion(strVersion)
    , m_pMainLayout(0)
    , m_pLabel(0)
{
    prepare();
}

void VBoxAboutDlg::showEvent(QShowEvent *pEvent)
{
    QDialog::showEvent(pEvent);
    if (!m_fPolished)
    {
        m_fPolished = true;
        QWidget *pParent = m_pPseudoParent ? m_pPseudoParent : parentWidget();
        const QRect available = gpDesktop->availableGeometry(pParent);
        resize(qMin(680, available.width() - 48), qMin(550, available.height() - 72));
        gpDesktop->centerWidget(this, pParent);
    }
}

void VBoxAboutDlg::sltRetranslateUI()
{
    setWindowTitle(tr("About"));
    if (m_pLabel)
        m_pLabel->setText(tr("Version %1 | Qt %2 (%3)").arg(m_strVersion, QString::fromLatin1(qVersion()),
                                                         QGuiApplication::platformName())
                         + QStringLiteral("<br/><br/>")
                         + tr("Built on the VirtualBox open-source platform.")
                         + QStringLiteral("<br/>")
                         + QStringLiteral("Copyright &copy; 2004-" VBOX_C_YEAR " " VBOX_VENDOR ".")
                         + QStringLiteral("<br/>")
                         + tr("Distributed under the <a style=\"color:%1\" href=\"https://www.gnu.org/licenses/gpl-3.0.html\">"
                              "GNU General Public License version 3</a>.").arg(uiAboutLinkColor(palette())));
}

void VBoxAboutDlg::prepare()
{
    if (m_pPseudoParent || parentWidget())
        setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowModality(Qt::WindowModal);
    setWindowIcon(UIBranding::icon());
    if (m_pPseudoParent)
        connect(m_pPseudoParent, &QObject::destroyed, this, &VBoxAboutDlg::close);
    prepareMainLayout();
    sltRetranslateUI();
    if (UITranslationEventListener::instance())
        connect(&translationEventListener(), &UITranslationEventListener::sigRetranslateUI,
                this, &VBoxAboutDlg::sltRetranslateUI);
}

void VBoxAboutDlg::prepareMainLayout()
{
    m_pMainLayout = new QVBoxLayout(this);
    m_pMainLayout->setContentsMargins(20, 20, 20, 16);
    m_pMainLayout->setSpacing(16);
    QFrame *pHero = new QFrame(this);
    pHero->setObjectName("naveboxAboutHero");
    pHero->setStyleSheet(QStringLiteral(
        "QFrame#naveboxAboutHero { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        " stop:0 #203b58,stop:1 #0b192a); border-radius: 12px; }"
        "QFrame#naveboxAboutHero QLabel { color: #edf3fa; background: transparent; }"
        "QLabel#naveboxSlogan { color: #bdcddd; }"));
    QHBoxLayout *pHeroLayout = new QHBoxLayout(pHero);
    pHeroLayout->setContentsMargins(18, 16, 18, 16);
    pHeroLayout->setSpacing(16);
    QLabel *pLogo = new QLabel(pHero);
    const qreal dRatio = gpDesktop->devicePixelRatio(this);
    pLogo->setPixmap(UIBranding::icon().pixmap(QSize(64, 64), dRatio));
    pHeroLayout->addWidget(pLogo);
    QVBoxLayout *pIdentity = new QVBoxLayout;
    pIdentity->setSpacing(4);
    pIdentity->addWidget(uiAboutLabel(UIBranding::name(), pHero, 17, true));
    QLabel *pSlogan = uiAboutLabel(UIBranding::slogan(), pHero, 1);
    pSlogan->setObjectName("naveboxSlogan");
    pIdentity->addWidget(pSlogan);
    pHeroLayout->addLayout(pIdentity, 1);
    m_pMainLayout->addWidget(pHero);
    preparePages();
    prepareCloseButton();
}

void VBoxAboutDlg::preparePages()
{
    QTabWidget *pTabs = new QTabWidget(this);
    QWidget *pOverview = new QWidget(pTabs);
    QVBoxLayout *pOverviewLayout = new QVBoxLayout(pOverview);
    pOverviewLayout->setContentsMargins(14, 12, 14, 12);
    pOverviewLayout->setSpacing(12);
    pOverviewLayout->addWidget(uiAboutLabel(tr("NAVE - The Virtualization Architecture"), pOverview, 2, true));
    pOverviewLayout->addWidget(uiAboutLabel(tr("FOUR PILLARS. ONE VIRTUAL WORLD."), pOverview));
    QGridLayout *pPillars = new QGridLayout;
    pPillars->setSpacing(14);
    const QStringList titles = QStringList() << tr("Network of Worlds") << tr("Architecture")
                                            << tr("Virtualization") << tr("Efficiency");
    const QStringList descriptions = QStringList()
        << tr("Connect independent virtual machines in one workspace.")
        << tr("A flexible, modular foundation for virtual environments.")
        << tr("Create, run and manage multiple systems on one computer.")
        << tr("Stay in control of compute, memory and storage resources.");
    const QString strLetters = QStringLiteral("NAVE");
    for (int i = 0; i < 4; ++i)
    {
        QFrame *pCard = new QFrame(pOverview);
        QVBoxLayout *pCardLayout = new QVBoxLayout(pCard);
        pCardLayout->setContentsMargins(0, 0, 8, 0);
        pCardLayout->setSpacing(5);
        pCardLayout->addWidget(uiAboutLabel(strLetters.mid(i, 1), pCard, 10, true));
        pCardLayout->addWidget(uiAboutLabel(titles.at(i), pCard, 0, true));
        pCardLayout->addWidget(uiAboutLabel(descriptions.at(i), pCard));
        pCardLayout->addStretch();
        pPillars->addWidget(pCard, i / 2, i % 2);
    }
    pPillars->setColumnStretch(0, 1);
    pPillars->setColumnStretch(1, 1);
    pOverviewLayout->addLayout(pPillars);
    QScrollArea *pStoryScroll = new QScrollArea(pTabs);
    pStoryScroll->setFrameShape(QFrame::NoFrame);
    pStoryScroll->setWidgetResizable(true);
    pStoryScroll->setWidget(pOverview);
    pTabs->addTab(pStoryScroll, tr("Overview"));

    QWidget *pContact = new QWidget(pTabs);
    QVBoxLayout *pContactLayout = new QVBoxLayout(pContact);
    pContactLayout->setContentsMargins(18, 18, 18, 18);
    pContactLayout->setSpacing(12);
    pContactLayout->addWidget(uiAboutLabel(tr("Contact"), pContact, 4, true));
    pContactLayout->addWidget(uiAboutLabel(UIBranding::author(), pContact, 1, true));
    QLabel *pEmail = uiAboutLabel(QStringLiteral("<a style=\"color:%2\" href=\"mailto:%1\">%1</a>")
                                .arg(UIBranding::email(), uiAboutLinkColor(palette())), pContact);
    pEmail->setOpenExternalLinks(true);
    pContactLayout->addWidget(pEmail);
    pContactLayout->addSpacing(8);
    m_pLabel = uiAboutLabel(QString(), pContact);
    m_pLabel->setOpenExternalLinks(true);
    pContactLayout->addWidget(m_pLabel);
    pContactLayout->addStretch();
    QScrollArea *pContactScroll = new QScrollArea(pTabs);
    pContactScroll->setFrameShape(QFrame::NoFrame);
    pContactScroll->setWidgetResizable(true);
    pContactScroll->setWidget(pContact);
    pTabs->addTab(pContactScroll, tr("Contact && Credits"));
    m_pMainLayout->addWidget(pTabs, 1);
}

void VBoxAboutDlg::prepareCloseButton()
{
    QIDialogButtonBox *pButtonBox = new QIDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
    connect(pButtonBox, &QDialogButtonBox::rejected, this, &VBoxAboutDlg::close);
    m_pMainLayout->addWidget(pButtonBox);
}
