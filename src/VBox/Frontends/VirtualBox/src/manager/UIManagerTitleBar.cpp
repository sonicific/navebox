/* $Id$ */
/** @file
 * VBox Qt GUI - Windows Manager title bar.
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
#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QPainter>
#include <QToolButton>
#include <QtMath>

/* GUI includes: */
#include "UIAppearance.h"
#include "UICommon.h"
#include "UIManagerTitleBar.h"
#include "UITranslationEventListener.h"

/* Windows includes: */
#include <iprt/cdefs.h>
#include <iprt/win/windows.h>
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:5039) /* dpa_dsa.h passes SDK callbacks with C linkage. */
#endif
#include <dwmapi.h>
#ifdef _MSC_VER
# pragma warning(pop)
#endif
#include <windowsx.h>


/** @page pg_ui_manager_title_bar Windows Manager Title Bar
 *
 * The Windows client area contains the menu and caption buttons in one row.
 * Windows handles the resize frame, system menu and drag region.
 */

/** Identifies the glyph drawn in a Manager caption button. */
enum ManagerCaptionAction
{
    ManagerCaptionAction_Minimize,
    ManagerCaptionAction_Maximize,
    ManagerCaptionAction_Close
};

/** Draws the Manager caption glyph at the current device scale. */
class QIManagerCaptionButton : public QToolButton
{
public:

    /** Creates a button for @a enmAction in @a pParent. */
    QIManagerCaptionButton(ManagerCaptionAction enmAction, QWidget *pParent)
        : QToolButton(pParent)
        , m_enmAction(enmAction)
    {
        setFixedSize(26, 28);
        setFocusPolicy(Qt::NoFocus);
        setProperty("managerCaptionButton", true);
    }

protected:

    /** Paints the button background and its icon. */
    virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE
    {
        QToolButton::paintEvent(pEvent);
        static const QRgb s_aFills[] = {0xffffbd2e, 0xff28c840, 0xffff5f57};
        static const QRgb s_aGlyphs[] = {0xff604100, 0xff104f1d, 0xff751a16};
        const bool fHovered = underMouse() || isDown();
        QColor fill = QColor::fromRgb(s_aFills[m_enmAction]);
        if (!window()->isActiveWindow() && !fHovered)
            fill = UIAppearance::PaneColors(QApplication::palette()).scrollThumb;
        if (isDown())
            fill = fill.darker(118);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(width() / 2.0, height() / 2.0);
        painter.setPen(QPen(fill.darker(113), 1));
        painter.setBrush(fill);
        painter.drawEllipse(QPointF(0, 0), 6.5, 6.5);

        if (!fHovered)
            return;
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor::fromRgb(s_aGlyphs[m_enmAction]), 1.15, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        switch (m_enmAction)
        {
            case ManagerCaptionAction_Minimize:
                painter.drawLine(QPointF(-3, 0), QPointF(3, 0));
                break;
            case ManagerCaptionAction_Maximize:
                if (window()->isMaximized())
                {
                    painter.drawRect(QRectF(-3, -1.5, 4.5, 4.5));
                    const QPointF aBack[] = {QPointF(-1.5, -1.5), QPointF(-1.5, -3), QPointF(3, -3),
                                             QPointF(3, 1.5), QPointF(1.5, 1.5)};
                    painter.drawPolyline(aBack, RT_ELEMENTS(aBack));
                }
                else
                    painter.drawRect(QRectF(-2.7, -2.7, 5.4, 5.4));
                break;
            case ManagerCaptionAction_Close:
                painter.drawLine(QPointF(-2.5, -2.5), QPointF(2.5, 2.5));
                painter.drawLine(QPointF(-2.5, 2.5), QPointF(2.5, -2.5));
                break;
        }
    }

private:

    /** Holds the caption button's purpose. */
    const ManagerCaptionAction m_enmAction;
};

UIManagerTitleBar::UIManagerTitleBar(QWidget *pParent)
    : UIMenuBar(pParent)
{
}

void UIManagerTitleBar::prepareFrame()
{
    if (QGuiApplication::platformName() != QLatin1String("windows"))
        return;

    HWND hWnd = reinterpret_cast<HWND>(window()->winId());
    if (!hWnd)
        return;

    setObjectName(QStringLiteral("managerTitleBar"));
    setMinimumHeight(34);
    m_pCaption = new QWidget(this);
    m_pCaption->setLayoutDirection(Qt::LeftToRight);
    QHBoxLayout *pLayout = new QHBoxLayout(m_pCaption);
    pLayout->setContentsMargins(4, 0, 7, 0);
    pLayout->setSpacing(0);
    m_apButtons[0] = new QIManagerCaptionButton(ManagerCaptionAction_Minimize, m_pCaption);
    m_apButtons[1] = new QIManagerCaptionButton(ManagerCaptionAction_Maximize, m_pCaption);
    m_apButtons[2] = new QIManagerCaptionButton(ManagerCaptionAction_Close, m_pCaption);
    for (QToolButton *pButton : m_apButtons)
        pLayout->addWidget(pButton);
    connect(m_apButtons[0], &QToolButton::clicked, this, [this] { window()->showMinimized(); });
    connect(m_apButtons[1], &QToolButton::clicked, this, [this]
    {
        if (window()->isMaximized())
            window()->showNormal();
        else
            window()->showMaximized();
    });
    connect(m_apButtons[2], &QToolButton::clicked, this, [this] { window()->close(); });
    connect(&translationEventListener(), &UITranslationEventListener::sigRetranslateUI,
            this, &UIManagerTitleBar::retranslateCaptionButtons);

    m_fExtended = true;
    updateCaptionCorner();
    retranslateCaptionButtons();
    updateAppearance();
    window()->installEventFilter(this);
    connect(&uiCommon(), &UICommon::sigThemeChange, this, &UIManagerTitleBar::updateAppearance);

    /* Recalculate the native frame after the menu bar can accept hit tests. */
    SetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

bool UIManagerTitleBar::handleFrameEvent(const QByteArray &strType, void *pMessage, qintptr *pResult)
{
    if (!m_fExtended || strType != "windows_generic_MSG")
        return false;

    const MSG &msg = *static_cast<const MSG*>(pMessage);
    switch (msg.message)
    {
        case WM_NCCALCSIZE:
        {
            if (!msg.wParam || IsIconic(msg.hwnd))
                return false;

            NCCALCSIZE_PARAMS *pParams = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg.lParam);
            const RECT frame = pParams->rgrc[0];
            DefWindowProcW(msg.hwnd, msg.message, msg.wParam, msg.lParam);

            /* Retain the native resize frame and replace the height used by the old caption. */
            pParams->rgrc[0].top = frame.top + (frame.bottom - pParams->rgrc[0].bottom);
            *pResult = 0;
            return true;
        }
        case WM_NCHITTEST:
        {
            POINT point = {GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam)};
            if (!ScreenToClient(msg.hwnd, &point))
                return false;

            const QPoint pos = mapFrom(window(), QPoint(qFloor(point.x / devicePixelRatioF()),
                                                        qFloor(point.y / devicePixelRatioF())));
            if (!rect().contains(pos))
                return false;

            *pResult = actionAt(pos) || childAt(pos) ? HTCLIENT : HTCAPTION;
            return true;
        }
        default:
            return false;
    }
}

bool UIManagerTitleBar::eventFilter(QObject *pObject, QEvent *pEvent)
{
    if (pObject == window())
    {
        if (pEvent->type() == QEvent::WindowStateChange)
            retranslateCaptionButtons();
        else if (pEvent->type() == QEvent::WindowActivate || pEvent->type() == QEvent::WindowDeactivate)
            for (QToolButton *pButton : m_apButtons)
                pButton->update();
    }
    return UIMenuBar::eventFilter(pObject, pEvent);
}

void UIManagerTitleBar::changeEvent(QEvent *pEvent)
{
    UIMenuBar::changeEvent(pEvent);
    if (m_fExtended && pEvent->type() == QEvent::LayoutDirectionChange)
        updateCaptionCorner();
}

void UIManagerTitleBar::updateAppearance()
{
    const UIAppearance::PaneColors colors(QApplication::palette());
    const BOOL fDark = !colors.fLight;
    DwmSetWindowAttribute(reinterpret_cast<HWND>(window()->effectiveWinId()),
                          DWMWA_USE_IMMERSIVE_DARK_MODE, &fDark, sizeof(fDark));
    const QString strStyle = QStringLiteral(
        "QMenuBar#managerTitleBar { background: %1; color: %2; padding: 0 5px; }"
        "QMenuBar#managerTitleBar::item { background: transparent; padding: 5px 9px; border-radius: 5px; }"
        "QMenuBar#managerTitleBar::item:selected { background: %3; }"
        "QMenuBar#managerTitleBar::item:pressed { background: %4; }"
        "QMenuBar#managerTitleBar QToolButton[managerCaptionButton=\"true\"] {"
        " background: transparent; border: none; }")
        .arg(colors.backdrop.name(), colors.text.name(), colors.hover.name(), colors.pressed.name());
    if (styleSheet() != strStyle)
        setStyleSheet(strStyle);
}

void UIManagerTitleBar::updateCaptionCorner()
{
    /* QMenuBar mirrors corner widgets in RTL. The window buttons stay on the right. */
    const Qt::Corner enmCorner = isRightToLeft() ? Qt::TopLeftCorner : Qt::TopRightCorner;
    const Qt::Corner enmOther = isRightToLeft() ? Qt::TopRightCorner : Qt::TopLeftCorner;
    if (cornerWidget(enmCorner) != m_pCaption)
    {
        if (cornerWidget(enmOther) == m_pCaption)
            setCornerWidget(0, enmOther);
        setCornerWidget(m_pCaption, enmCorner);
    }
}

void UIManagerTitleBar::retranslateCaptionButtons()
{
    const QString aLabels[] = {tr("Minimize"), window()->isMaximized() ? tr("Restore") : tr("Maximize"), tr("Close")};
    for (unsigned i = 0; i < RT_ELEMENTS(m_apButtons); ++i)
    {
        m_apButtons[i]->setToolTip(aLabels[i]);
        m_apButtons[i]->setAccessibleName(aLabels[i]);
        m_apButtons[i]->update();
    }
}
