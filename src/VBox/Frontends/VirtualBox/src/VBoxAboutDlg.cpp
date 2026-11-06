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
#include <QClipboard>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIDialogButtonBox.h"
#include "UIBranding.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UITranslationEventListener.h"
#include "VBoxAboutDlg.h"

/* Other VBox includes: */
#include <VBox/version.h>
#include <iprt/buildconfig.h>

/** @page pg_ui_navebox_about Navebox Product Information
 *
 * Present the product identity, the four NAVE pillars, contact details, build
 * information and upstream attribution together. The page scrolls on small
 * screens while the action buttons stay accessible.
 */

/** Adds a word-wrapped label with the requested type hierarchy. */
static QLabel *uiAboutLabel(const QString &strText, QWidget *pParent, int iPointDelta = 0, bool fBold = false)
{
    QLabel *pLabel = new QLabel(strText, pParent);
    pLabel->setTextFormat(Qt::PlainText);
    pLabel->setWordWrap(true);
    QFont labelFont = pLabel->font();
    if (labelFont.pointSize() > 0)
        labelFont.setPointSize(qMax(8, labelFont.pointSize() + iPointDelta));
    else
        labelFont.setPixelSize(qMax(12, labelFont.pixelSize() + iPointDelta));
    labelFont.setBold(fBold);
    pLabel->setFont(labelFont);
    return pLabel;
}

/** Makes a rich-text link label accessible by keyboard and mouse. */
static void uiAboutEnableLinks(QLabel *pLabel)
{
    pLabel->setTextFormat(Qt::RichText);
    pLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
    pLabel->setOpenExternalLinks(true);
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
    , m_strVersion(strVersion.isEmpty() ? QString::fromLatin1(RTBldCfgVersion()) : strVersion)
    , m_pVersion(0)
    , m_pAttribution(0)
    , m_pOverviewHeading(0)
    , m_pOverviewIntroduction(0)
    , m_apPillarTitles{0, 0, 0, 0}
    , m_apPillarDescriptions{0, 0, 0, 0}
    , m_pContactHeading(0)
    , m_pEmail(0)
    , m_pBuildInformation(0)
    , m_pCopyBuildInformation(0)
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
        resize(qMin(700, qMax(1, available.width() - 48)), qMin(570, qMax(1, available.height() - 72)));
        gpDesktop->centerWidget(this, pParent);
    }
}

void VBoxAboutDlg::changeEvent(QEvent *pEvent)
{
    QDialog::changeEvent(pEvent);
    if (   pEvent->type() == QEvent::LanguageChange
        || pEvent->type() == QEvent::PaletteChange
        || pEvent->type() == QEvent::ApplicationPaletteChange)
        sltRetranslateUI();
}

void VBoxAboutDlg::sltRetranslateUI()
{
    setWindowTitle(tr("About"));
    if (m_pVersion)
        m_pVersion->setText(tr("Version %1 | Qt %2 (%3)")
                            .arg(m_strVersion, QString::fromLatin1(qVersion()), QGuiApplication::platformName()));
    if (m_pOverviewHeading)
        m_pOverviewHeading->setText(tr("NAVE - The Virtualization Architecture"));
    if (m_pOverviewIntroduction)
        m_pOverviewIntroduction->setText(tr("FOUR PILLARS. ONE VIRTUAL WORLD."));
    const QStringList titles = QStringList() << tr("Network of Worlds") << tr("Architecture")
                                            << tr("Virtualization") << tr("Efficiency");
    const QStringList descriptions = QStringList()
        << tr("Connect independent virtual machines in one workspace.")
        << tr("A flexible, modular foundation for virtual environments.")
        << tr("Create, run and manage multiple systems on one computer.")
        << tr("Stay in control of compute, memory and storage resources.");
    for (int i = 0; i < 4; ++i)
    {
        if (m_apPillarTitles[i])
            m_apPillarTitles[i]->setText(titles.at(i));
        if (m_apPillarDescriptions[i])
            m_apPillarDescriptions[i]->setText(descriptions.at(i));
    }
    if (m_pContactHeading)
        m_pContactHeading->setText(tr("Contact"));
    if (m_pEmail)
        m_pEmail->setText(QStringLiteral("<a style=\"color:%2\" href=\"mailto:%1\">%1</a>")
                         .arg(UIBranding::email().toHtmlEscaped(), uiAboutLinkColor(palette())));
    if (m_pCopyBuildInformation)
        m_pCopyBuildInformation->setText(tr("Copy Build Information"));
    if (m_pBuildInformation)
    {
        QString strBuildStatus;
#ifdef VBOX_BLEEDING_EDGE
        strBuildStatus = tr("EXPERIMENTAL build %1").arg(QStringLiteral(VBOX_BLEEDING_EDGE));
#elif VBOX_VERSION_BUILD % 2
        strBuildStatus = tr("Development build");
#endif
        const QString strBuildDetails = tr("Build type: %1; source revision: %2")
                                        .arg(QString::fromLatin1(RTBldCfgType())).arg(RTBldCfgRevision());
        m_pBuildInformation->setText(strBuildStatus.isEmpty() ? strBuildDetails
                                                            : strBuildStatus + QLatin1Char('\n') + strBuildDetails);
    }
    if (m_pAttribution)
        m_pAttribution->setText(tr("Built on the VirtualBox open-source platform.")
                                + QStringLiteral("<br/>Copyright &copy; 2004-" VBOX_C_YEAR " " VBOX_VENDOR ". ")
                                + tr("Distributed under the <a style=\"color:%1\" "
                                     "href=\"https://www.gnu.org/licenses/gpl-3.0.html\">"
                                     "GNU General Public License version 3</a>.").arg(uiAboutLinkColor(palette())));
}

QString VBoxAboutDlg::buildInformation() const
{
    /* Keep support keys stable across languages.  Do not include machine paths or VM data. */
    return QStringLiteral("%1 %2\nVirtualBox %3r%4\nBuild: %5 (%6)\nQt: %7 (%8)")
           .arg(UIBranding::name(), m_strVersion, QString::fromLatin1(RTBldCfgVersion()))
           .arg(RTBldCfgRevision())
           .arg(QString::fromLatin1(RTBldCfgType()), QString::fromLatin1(RTBldCfgTargetDotArch()),
                QString::fromLatin1(qVersion()), QGuiApplication::platformName());
}

void VBoxAboutDlg::sltCopyBuildInformation()
{
    QApplication::clipboard()->setText(buildInformation());
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
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    pMainLayout->setContentsMargins(20, 20, 20, 16);
    pMainLayout->setSpacing(14);

    QScrollArea *pScroll = new QScrollArea(this);
    pScroll->setObjectName("aboutScroll");
    pScroll->setFrameShape(QFrame::NoFrame);
    pScroll->setWidgetResizable(true);
    QWidget *pPage = new QWidget;
    pPage->setBackgroundRole(QPalette::Window);
    QVBoxLayout *pPageLayout = new QVBoxLayout(pPage);
    pPageLayout->setContentsMargins(0, 0, 0, 0);
    pPageLayout->setSpacing(14);
    prepareHero(pPage, pPageLayout);
    prepareOverview(pPage, pPageLayout);
    prepareSupport(pPage, pPageLayout);
    pPageLayout->addStretch();
    pScroll->setWidget(pPage);
    pMainLayout->addWidget(pScroll, 1);
    prepareActions(pMainLayout);
}

void VBoxAboutDlg::prepareHero(QWidget *pPage, QVBoxLayout *pLayout)
{
    QFrame *pHero = new QFrame(pPage);
    pHero->setObjectName("naveboxAboutHero");
    pHero->setStyleSheet(QStringLiteral(
        "QFrame#naveboxAboutHero { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        " stop:0 #203b58,stop:1 #0b192a); border-radius: 12px; }"
        "QFrame#naveboxAboutHero QLabel { color: #edf3fa; background: transparent; }"
        "QLabel#naveboxSlogan, QLabel#naveboxVersion { color: #bdcddd; }"));
    QHBoxLayout *pHeroLayout = new QHBoxLayout(pHero);
    pHeroLayout->setContentsMargins(18, 16, 18, 16);
    pHeroLayout->setSpacing(16);
    QLabel *pLogo = new QLabel(pHero);
    const qreal dRatio = gpDesktop->devicePixelRatio(this);
    pLogo->setPixmap(UIBranding::icon().pixmap(QSize(64, 64), dRatio));
    pLogo->setAccessibleName(UIBranding::name());
    pHeroLayout->addWidget(pLogo);
    QVBoxLayout *pIdentity = new QVBoxLayout;
    pIdentity->setSpacing(4);
    pIdentity->addWidget(uiAboutLabel(UIBranding::name(), pHero, 17, true));
    QLabel *pSlogan = uiAboutLabel(UIBranding::slogan(), pHero, 1);
    pSlogan->setObjectName("naveboxSlogan");
    pIdentity->addWidget(pSlogan);
    m_pVersion = uiAboutLabel(QString(), pHero, 0, true);
    m_pVersion->setObjectName("naveboxVersion");
    pIdentity->addWidget(m_pVersion);
    pHeroLayout->addLayout(pIdentity, 1);
    pLayout->addWidget(pHero);
}

void VBoxAboutDlg::prepareOverview(QWidget *pPage, QVBoxLayout *pLayout)
{
    QWidget *pOverview = new QWidget(pPage);
    QVBoxLayout *pOverviewLayout = new QVBoxLayout(pOverview);
    pOverviewLayout->setContentsMargins(0, 0, 0, 0);
    pOverviewLayout->setSpacing(6);
    m_pOverviewHeading = uiAboutLabel(QString(), pOverview, 2, true);
    m_pOverviewIntroduction = uiAboutLabel(QString(), pOverview);
    pOverviewLayout->addWidget(m_pOverviewHeading);
    pOverviewLayout->addWidget(m_pOverviewIntroduction);
    pOverviewLayout->addSpacing(4);
    QGridLayout *pPillars = new QGridLayout;
    pPillars->setSpacing(8);
    const QString strLetters = QStringLiteral("NAVE");
    for (int i = 0; i < 4; ++i)
    {
        QFrame *pCard = new QFrame(pOverview);
        pCard->setObjectName("aboutPillar");
        pCard->setStyleSheet(QStringLiteral(
            "QFrame#aboutPillar { background: palette(base); border: 1px solid palette(midlight); border-radius: 8px; }"
            "QLabel#aboutPillarLetter { background: #203b58; color: #edf3fa; border-radius: 6px; }"));
        QHBoxLayout *pCardLayout = new QHBoxLayout(pCard);
        pCardLayout->setContentsMargins(10, 10, 10, 10);
        pCardLayout->setSpacing(10);
        QLabel *pLetter = uiAboutLabel(strLetters.mid(i, 1), pCard, 2, true);
        pLetter->setObjectName("aboutPillarLetter");
        pLetter->setAlignment(Qt::AlignCenter);
        const int iLetterSize = pLetter->fontMetrics().height() + 12;
        pLetter->setFixedSize(iLetterSize, iLetterSize);
        pCardLayout->addWidget(pLetter, 0, Qt::AlignTop);
        QVBoxLayout *pCardText = new QVBoxLayout;
        pCardText->setSpacing(3);
        m_apPillarTitles[i] = uiAboutLabel(QString(), pCard, 0, true);
        m_apPillarDescriptions[i] = uiAboutLabel(QString(), pCard);
        pCardText->addWidget(m_apPillarTitles[i]);
        pCardText->addWidget(m_apPillarDescriptions[i]);
        pCardText->addStretch();
        pCardLayout->addLayout(pCardText, 1);
        pPillars->addWidget(pCard, i / 2, i % 2);
    }
    pPillars->setColumnStretch(0, 1);
    pPillars->setColumnStretch(1, 1);
    pOverviewLayout->addLayout(pPillars);
    pLayout->addWidget(pOverview);
}

void VBoxAboutDlg::prepareSupport(QWidget *pPage, QVBoxLayout *pLayout)
{
    QFrame *pSupport = new QFrame(pPage);
    pSupport->setObjectName("aboutSupport");
    pSupport->setStyleSheet(QStringLiteral(
        "QFrame#aboutSupport { background: palette(base); border: 1px solid palette(midlight); border-radius: 8px; }"));
    QVBoxLayout *pSupportLayout = new QVBoxLayout(pSupport);
    pSupportLayout->setContentsMargins(14, 12, 14, 12);
    pSupportLayout->setSpacing(10);
    QHBoxLayout *pDetails = new QHBoxLayout;
    pDetails->setSpacing(16);
    pSupportLayout->addLayout(pDetails);

    QVBoxLayout *pContact = new QVBoxLayout;
    pContact->setSpacing(4);
    m_pContactHeading = uiAboutLabel(QString(), pSupport, 1, true);
    pContact->addWidget(m_pContactHeading);
    pContact->addWidget(uiAboutLabel(UIBranding::author(), pSupport));
    m_pEmail = uiAboutLabel(QString(), pSupport);
    uiAboutEnableLinks(m_pEmail);
    pContact->addWidget(m_pEmail);
    pContact->addStretch();
    pDetails->addLayout(pContact, 1);

    m_pBuildInformation = uiAboutLabel(QString(), pSupport);
    m_pBuildInformation->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pDetails->addWidget(m_pBuildInformation, 1, Qt::AlignTop);

    QFrame *pSeparator = new QFrame(pSupport);
    pSeparator->setFrameShape(QFrame::HLine);
    pSeparator->setStyleSheet(QStringLiteral("background: palette(midlight); border: none;"));
    pSeparator->setFixedHeight(1);
    pSupportLayout->addWidget(pSeparator);
    m_pAttribution = uiAboutLabel(QString(), pSupport);
    uiAboutEnableLinks(m_pAttribution);
    pSupportLayout->addWidget(m_pAttribution);
    pLayout->addWidget(pSupport);
}

void VBoxAboutDlg::prepareActions(QVBoxLayout *pLayout)
{
    QIDialogButtonBox *pButtonBox = new QIDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
    m_pCopyBuildInformation = pButtonBox->addButton(QString(), QDialogButtonBox::ActionRole);
    m_pCopyBuildInformation->setObjectName("copyBuildInformationButton");
    m_pCopyBuildInformation->setAutoDefault(false);
    connect(m_pCopyBuildInformation, &QPushButton::clicked, this, &VBoxAboutDlg::sltCopyBuildInformation);
    connect(pButtonBox, &QDialogButtonBox::rejected, this, &VBoxAboutDlg::close);
    pLayout->addWidget(pButtonBox);
}
