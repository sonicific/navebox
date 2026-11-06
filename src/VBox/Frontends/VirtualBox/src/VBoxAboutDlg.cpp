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
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIDialogButtonBox.h"
#include "UIAppearance.h"
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

/** Draws the blue product banner and the layered VM illustration behind its labels. */
class UIAboutHero : public QFrame
{
public:
    explicit UIAboutHero(QWidget *pParent) : QFrame(pParent) {}

protected:
    void paintEvent(QPaintEvent *) RT_OVERRIDE
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        if (isRightToLeft())
        {
            painter.translate(width(), 0);
            painter.scale(-1, 1);
        }
        QPainterPath bounds;
        bounds.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 15, 15);
        QLinearGradient fill(0, height(), width(), 0);
        fill.setColorAt(0, QColor("#102f6d"));
        fill.setColorAt(0.53, QColor("#174aa0"));
        fill.setColorAt(1, QColor("#508eef"));
        painter.fillPath(bounds, fill);
        painter.setClipPath(bounds);

        if (width() < 720)
            return;
        painter.translate(width() - 290, height() - 144);
        QPainterPath wave;
        wave.moveTo(-100, 144);
        wave.cubicTo(15, 140, 75, -3, 290, 1);
        wave.lineTo(290, 144);
        wave.closeSubpath();
        painter.fillPath(wave, QColor(155, 204, 255, 70));
        painter.translate(23, 58);
        painter.rotate(-9);
        for (int i = 0; i < 3; ++i)
        {
            const QRectF panel(i * 25, 16 - i * 10, 166, 105);
            QLinearGradient glass(panel.topLeft(), panel.bottomRight());
            glass.setColorAt(0, QColor(165, 211, 255, 220));
            glass.setColorAt(1, QColor(78, 142, 230, 190));
            painter.setPen(QPen(QColor(226, 244, 255, 150), 2));
            painter.setBrush(glass);
            painter.drawRoundedRect(panel, 12, 12);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(223, 242, 255, 175));
            for (int j = 0; j < 3; ++j)
                painter.drawEllipse(panel.topLeft() + QPointF(12 + j * 8, 13), 2, 2);
        }
        painter.translate(136, 51);
        const QPointF top[] = {{-22, -12}, {0, -24}, {22, -12}, {0, 0}};
        const QPointF left[] = {{-22, -12}, {0, 0}, {0, 26}, {-22, 14}};
        const QPointF right[] = {{0, 0}, {22, -12}, {22, 14}, {0, 26}};
        painter.setBrush(QColor("#d2ecff"));
        painter.drawPolygon(top, 4);
        painter.setBrush(QColor("#83bdff"));
        painter.drawPolygon(left, 4);
        painter.setBrush(QColor("#3284ec"));
        painter.drawPolygon(right, 4);
    }
};

/** Draws the four pillar glyphs and the two support symbols. */
static QPixmap uiAboutBadge(int iBadge, qreal fDpr)
{
    const int iSize = iBadge < 4 ? 34 : 30;
    QPixmap pixmap(qRound(iSize * fDpr), qRound(iSize * fDpr));
    pixmap.setDevicePixelRatio(fDpr);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(iSize / 46.0, iSize / 46.0);
    static const QRgb s_aColors[] = {0xff2165e8, 0xff6347df, 0xff0b9b63, 0xffad5a07, 0xff2165e8, 0xff2165e8};
    const QColor color = QColor::fromRgb(s_aColors[iBadge]);
    QLinearGradient fill(0, 0, 46, 46);
    fill.setColorAt(0, color.lighter(114));
    fill.setColorAt(1, color);
    painter.setPen(Qt::NoPen);
    painter.setBrush(fill);
    painter.drawRoundedRect(QRectF(0.5, 0.5, 45, 45), 11, 11);
    painter.setPen(QPen(Qt::white, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    switch (iBadge)
    {
        case 0:
            painter.drawEllipse(QRectF(12, 12, 22, 22));
            painter.drawEllipse(QRectF(18, 12, 10, 22));
            painter.drawLine(QPointF(12, 23), QPointF(34, 23));
            break;
        case 1:
        {
            QFont font = painter.font();
            font.setBold(true);
            font.setPixelSize(27);
            painter.setFont(font);
            painter.drawText(QRectF(0, 0, 46, 46), Qt::AlignCenter, QStringLiteral("A"));
            break;
        }
        case 2:
            painter.drawRoundedRect(QRectF(12, 13, 22, 15), 2, 2);
            painter.drawLine(QPointF(23, 28), QPointF(23, 33));
            painter.drawLine(QPointF(17, 33), QPointF(29, 33));
            break;
        case 3:
        {
            const QPointF bolt[] = {{25, 10}, {14, 25}, {22, 25}, {20, 36}, {32, 20}, {24, 20}};
            painter.setBrush(Qt::white);
            painter.drawPolygon(bolt, 6);
            break;
        }
        case 4:
        {
            painter.drawRoundedRect(QRectF(10, 14, 26, 18), 2, 2);
            const QPointF fold[] = {{11, 16}, {23, 25}, {35, 16}};
            painter.drawPolyline(fold, 3);
            break;
        }
        case 5:
        {
            const QPointF left[] = {{17, 16}, {10, 23}, {17, 30}};
            const QPointF right[] = {{29, 16}, {36, 23}, {29, 30}};
            painter.drawPolyline(left, 3);
            painter.drawPolyline(right, 3);
            painter.drawLine(QPointF(25, 15), QPointF(21, 31));
            break;
        }
    }
    painter.end();
    return pixmap;
}

/** Adds a support card with a leading icon and returns its text column. */
static QVBoxLayout *uiAboutSupportCard(QWidget *pParent, QHBoxLayout *pCards, int iBadge, int iStretch, qreal fDpr)
{
    QFrame *pCard = new QFrame(pParent);
    pCard->setObjectName(QStringLiteral("aboutSupport"));
    QHBoxLayout *pLayout = new QHBoxLayout(pCard);
    pLayout->setContentsMargins(13, 10, 13, 10);
    pLayout->setSpacing(10);
    QLabel *pBadge = new QLabel(pCard);
    pBadge->setPixmap(uiAboutBadge(iBadge, fDpr));
    pLayout->addWidget(pBadge, 0, Qt::AlignTop);
    QVBoxLayout *pText = new QVBoxLayout;
    pText->setSpacing(3);
    pLayout->addLayout(pText, 1);
    pCards->addWidget(pCard, iStretch);
    return pText;
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
        resize(qMin(880, qMax(1, available.width() - 48)), qMin(600, qMax(1, available.height() - 72)));
        gpDesktop->centerWidget(this, pParent);
    }
}

void VBoxAboutDlg::changeEvent(QEvent *pEvent)
{
    QDialog::changeEvent(pEvent);
    if (pEvent->type() == QEvent::LanguageChange)
        sltRetranslateUI();
    else if (pEvent->type() == QEvent::PaletteChange || pEvent->type() == QEvent::ApplicationPaletteChange)
        QMetaObject::invokeMethod(this, &VBoxAboutDlg::updateAppearance, Qt::QueuedConnection);
}

void VBoxAboutDlg::sltRetranslateUI()
{
    setWindowTitle(QStringLiteral("%1 - %2").arg(tr("About"), UIBranding::name()));
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
                                                            : strBuildStatus + QStringLiteral("  \u00b7  ") + strBuildDetails);
    }
    if (m_pAttribution)
        m_pAttribution->setText(tr("Built on the VirtualBox open-source platform.")
                                + QStringLiteral("<br/>Copyright &copy; 2004-" VBOX_C_YEAR " " VBOX_VENDOR ".<br/>")
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
    updateAppearance();
    if (UITranslationEventListener::instance())
        connect(&translationEventListener(), &UITranslationEventListener::sigRetranslateUI,
                this, &VBoxAboutDlg::sltRetranslateUI);
}

void VBoxAboutDlg::prepareMainLayout()
{
    setObjectName(QStringLiteral("naveboxAboutDialog"));
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    pMainLayout->setContentsMargins(16, 12, 16, 12);
    pMainLayout->setSpacing(10);

    QScrollArea *pScroll = new QScrollArea(this);
    pScroll->setObjectName("aboutScroll");
    pScroll->setFrameShape(QFrame::NoFrame);
    pScroll->setWidgetResizable(true);
    QWidget *pPage = new QWidget;
    pPage->setObjectName(QStringLiteral("aboutPage"));
    pPage->setAttribute(Qt::WA_StyledBackground);
    QVBoxLayout *pPageLayout = new QVBoxLayout(pPage);
    pPageLayout->setContentsMargins(0, 0, 0, 0);
    pPageLayout->setSpacing(12);
    prepareHero(pPage, pPageLayout);
    prepareOverview(pPage, pPageLayout);
    prepareSupport(pPage, pPageLayout);
    pPageLayout->addStretch();
    pScroll->setWidget(pPage);
    pMainLayout->addWidget(pScroll, 1);
    prepareActions(pMainLayout);
}

void VBoxAboutDlg::updateAppearance()
{
    const UIAppearance::PaneColors colors(palette());
    const QString strStyle = QStringLiteral(
        "QDialog#naveboxAboutDialog, QWidget#aboutPage, QScrollArea#aboutScroll { background: %1; border: none; }"
        "QDialog#naveboxAboutDialog QLabel { color: %2; }"
        "QFrame#naveboxAboutHero QLabel { color: #ffffff; }"
        "QFrame#naveboxAboutHero QLabel#naveboxSlogan { color: #dbe9ff; }"
        "QFrame#naveboxAboutHero QLabel#naveboxVersion {"
        " background: rgba(219,235,255,45); border-radius: 11px; }"
        "QFrame#naveboxAboutHero QFrame#aboutDivider { background: #a9c9ff; }"
        "QDialog#naveboxAboutDialog QLabel[aboutSecondary=\"true\"] { color: %3; }"
        "QDialog#naveboxAboutDialog QLabel#aboutOverviewEyebrow { color: %7; }"
        "QFrame#aboutPillar { background: %4; border: 1px solid %5; border-radius: 13px; }"
        "QFrame#aboutSupport { background: %6; border-radius: 11px; }"
        "QFrame#aboutDivider { background: %5; border: none; }"
        "QDialog#naveboxAboutDialog QPushButton { color: %2; background: %4;"
        " border: 1px solid %5; border-radius: 14px; padding: 7px 15px; font-weight: 600; }"
        "QDialog#naveboxAboutDialog QPushButton:hover { background: %6; }"
        "QDialog#naveboxAboutDialog QPushButton:focus { border-color: palette(highlight); }"
        "QDialog#naveboxAboutDialog QPushButton#aboutClose { background: #2165df; border-color: #2165df; color: white; }"
        "QDialog#naveboxAboutDialog QPushButton#aboutClose:hover { background: #1c56c0; }"
        "QDialog#naveboxAboutDialog QPushButton#aboutClose:pressed { background: #1748a2; }"
        "QDialog#naveboxAboutDialog QPushButton#aboutClose:focus { border-color: #a9ceff; }")
        .arg(colors.backdrop.name(), colors.text.name(), colors.secondary.name(),
             colors.surface.name(), colors.border.name(), colors.hover.name(),
             colors.fLight ? QStringLiteral("#3d5d93") : QStringLiteral("#a8c3f5"));
    if (styleSheet() != strStyle)
        setStyleSheet(strStyle);
    sltRetranslateUI();
}

void VBoxAboutDlg::prepareHero(QWidget *pPage, QVBoxLayout *pLayout)
{
    QFrame *pHero = new UIAboutHero(pPage);
    pHero->setObjectName("naveboxAboutHero");
    QHBoxLayout *pHeroLayout = new QHBoxLayout(pHero);
    pHeroLayout->setContentsMargins(22, 16, 22, 16);
    pHeroLayout->setSpacing(17);
    QLabel *pLogo = new QLabel(pHero);
    pLogo->setObjectName(QStringLiteral("naveboxAboutLogo"));
    const qreal dRatio = gpDesktop->devicePixelRatio(this);
    pLogo->setPixmap(UIBranding::icon().pixmap(QSize(76, 76), dRatio));
    pLogo->setAccessibleName(UIBranding::name());
    pHeroLayout->addWidget(pLogo, 0, Qt::AlignVCenter);
    QFrame *pDivider = new QFrame(pHero);
    pDivider->setObjectName(QStringLiteral("aboutDivider"));
    pDivider->setFixedSize(1, 64);
    pHeroLayout->addWidget(pDivider, 0, Qt::AlignVCenter);
    QVBoxLayout *pIdentity = new QVBoxLayout;
    pIdentity->setSpacing(4);
    pIdentity->addWidget(uiAboutLabel(UIBranding::name(), pHero, 23, true));
    QLabel *pSlogan = uiAboutLabel(UIBranding::slogan(), pHero);
    pSlogan->setObjectName("naveboxSlogan");
    pSlogan->setWordWrap(false);
    pIdentity->addWidget(pSlogan);
    m_pVersion = uiAboutLabel(QString(), pHero, 0, true);
    m_pVersion->setObjectName("naveboxVersion");
    m_pVersion->setWordWrap(false);
    m_pVersion->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    m_pVersion->setMargin(5);
    pIdentity->addWidget(m_pVersion);
    pHeroLayout->addLayout(pIdentity);
    pHeroLayout->addStretch(1);
    pLayout->addWidget(pHero);
}

void VBoxAboutDlg::prepareOverview(QWidget *pPage, QVBoxLayout *pLayout)
{
    QWidget *pOverview = new QWidget(pPage);
    QVBoxLayout *pOverviewLayout = new QVBoxLayout(pOverview);
    pOverviewLayout->setContentsMargins(0, 0, 0, 0);
    pOverviewLayout->setSpacing(4);
    m_pOverviewIntroduction = uiAboutLabel(QString(), pOverview, -1, true);
    m_pOverviewIntroduction->setObjectName(QStringLiteral("aboutOverviewEyebrow"));
    QFont eyebrowFont = m_pOverviewIntroduction->font();
    eyebrowFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    m_pOverviewIntroduction->setFont(eyebrowFont);
    pOverviewLayout->addWidget(m_pOverviewIntroduction);
    QHBoxLayout *pHeadingRow = new QHBoxLayout;
    pHeadingRow->setSpacing(12);
    m_pOverviewHeading = uiAboutLabel(QString(), pOverview, 5, true);
    pHeadingRow->addWidget(m_pOverviewHeading, 3);
    QFrame *pHeadingRule = new QFrame(pOverview);
    pHeadingRule->setObjectName(QStringLiteral("aboutDivider"));
    pHeadingRule->setFixedHeight(1);
    pHeadingRow->addWidget(pHeadingRule, 2, Qt::AlignVCenter);
    pOverviewLayout->addLayout(pHeadingRow);
    pOverviewLayout->addSpacing(5);
    QGridLayout *pPillars = new QGridLayout;
    pPillars->setSpacing(10);
    for (int i = 0; i < 4; ++i)
    {
        QFrame *pCard = new QFrame(pOverview);
        pCard->setObjectName("aboutPillar");
        QHBoxLayout *pCardLayout = new QHBoxLayout(pCard);
        pCardLayout->setContentsMargins(13, 10, 13, 10);
        pCardLayout->setSpacing(11);
        QLabel *pBadge = new QLabel(pCard);
        pBadge->setPixmap(uiAboutBadge(i, gpDesktop->devicePixelRatio(this)));
        pCardLayout->addWidget(pBadge, 0, Qt::AlignVCenter);
        QVBoxLayout *pCardText = new QVBoxLayout;
        pCardText->setSpacing(3);
        m_apPillarTitles[i] = uiAboutLabel(QString(), pCard, 2, true);
        m_apPillarDescriptions[i] = uiAboutLabel(QString(), pCard);
        m_apPillarDescriptions[i]->setProperty("aboutSecondary", true);
        pCardText->addStretch();
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
    QHBoxLayout *pDetails = new QHBoxLayout;
    pDetails->setSpacing(10);
    const qreal fDpr = gpDesktop->devicePixelRatio(this);
    QVBoxLayout *pContact = uiAboutSupportCard(pPage, pDetails, 4, 2, fDpr);
    QWidget *pContactCard = pContact->parentWidget();
    m_pContactHeading = uiAboutLabel(QString(), pContactCard, -1, true);
    m_pContactHeading->setProperty("aboutSecondary", true);
    pContact->addWidget(m_pContactHeading);
    pContact->addWidget(uiAboutLabel(UIBranding::author(), pContactCard, 1, true));
    m_pEmail = uiAboutLabel(QString(), pContactCard);
    uiAboutEnableLinks(m_pEmail);
    pContact->addWidget(m_pEmail);
    pContact->addStretch();

    QVBoxLayout *pCredits = uiAboutSupportCard(pPage, pDetails, 5, 3, fDpr);
    m_pAttribution = uiAboutLabel(QString(), pCredits->parentWidget());
    uiAboutEnableLinks(m_pAttribution);
    pCredits->addWidget(m_pAttribution);
    pCredits->addStretch();
    pLayout->addLayout(pDetails);

    m_pBuildInformation = uiAboutLabel(QString(), pPage, -1);
    m_pBuildInformation->setProperty("aboutSecondary", true);
    m_pBuildInformation->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pLayout->addWidget(m_pBuildInformation);
}

void VBoxAboutDlg::prepareActions(QVBoxLayout *pLayout)
{
    QIDialogButtonBox *pButtonBox = new QIDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
    pButtonBox->button(QDialogButtonBox::Close)->setObjectName(QStringLiteral("aboutClose"));
    m_pCopyBuildInformation = pButtonBox->addButton(QString(), QDialogButtonBox::ActionRole);
    m_pCopyBuildInformation->setObjectName("copyBuildInformationButton");
    m_pCopyBuildInformation->setAutoDefault(false);
    connect(m_pCopyBuildInformation, &QPushButton::clicked, this, &VBoxAboutDlg::sltCopyBuildInformation);
    connect(pButtonBox, &QDialogButtonBox::rejected, this, &VBoxAboutDlg::close);
    pLayout->addWidget(pButtonBox);
}
