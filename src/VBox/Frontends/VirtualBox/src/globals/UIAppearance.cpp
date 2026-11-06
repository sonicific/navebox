/* $Id$ */
/** @file
 * VBox Qt GUI - Shared palette-aware control appearance.
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
#include <QPainter>

/* GUI includes: */
#include "UIAppearance.h"

/** @page pg_ui_appearance Common Control Appearance
 *
 * Replace native bevels with a thin frame while retaining palette colors,
 * keyboard focus, disabled-state contrast and the native control behavior.
 * Layout, validation and control-specific interaction remain with each widget.
 */

/** Converts a palette color into a stylesheet color with the requested opacity. */
static QString uiAppearanceColor(QColor color, int iAlpha)
{
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(color.red()).arg(color.green()).arg(color.blue()).arg(iAlpha);
}

/* static */
QString UIAppearance::styleSheet(const QPalette &palette)
{
    const QString strBorder = uiAppearanceColor(palette.color(QPalette::WindowText), 38);
    const QString strDisabledBorder = uiAppearanceColor(palette.color(QPalette::WindowText), 20);
    const QString strArrowTheme = palette.color(QPalette::Base).lightness() < 128
                                ? QStringLiteral("dark") : QStringLiteral("light");
    return QStringLiteral(
        "QLineEdit, QAbstractSpinBox, QComboBox {"
        " background: palette(base); border: 1px solid %1; border-radius: 5px; padding: 3px 6px; }"
        "QLineEdit:focus, QAbstractSpinBox:focus, QComboBox:focus { border-color: palette(highlight); }"
        "QLineEdit:disabled, QAbstractSpinBox:disabled, QComboBox:disabled { border-color: %2; }"
        "QAbstractSpinBox QLineEdit, QComboBox QLineEdit { border: none; border-radius: 0;"
        " padding: 0; background: transparent; }"
        "QComboBox::drop-down { border: none; background: transparent; width: 20px; }"
        "QAbstractSpinBox::up-button, QAbstractSpinBox::down-button {"
        " border: none; background: transparent; width: 18px; }"
        "QComboBox::down-arrow, QAbstractSpinBox::down-arrow {"
        " image: url(:/navebox-chevron-down-%3.png); width: 10px; height: 7px; }"
        "QAbstractSpinBox::up-arrow {"
        " image: url(:/navebox-chevron-up-%3.png); width: 10px; height: 7px; }"
        "QGroupBox { border: 1px solid %1; border-radius: 7px; margin-top: 0.8em; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left;"
        " left: 10px; padding: 0 4px; }"
        "QToolTip { color: palette(tool-tip-text); background: palette(tool-tip-base);"
        " border: 1px solid %1; padding: 4px 6px; }").arg(strBorder, strDisabledBorder, strArrowTheme);
}

/* static */
void UIAppearance::paintSheet(QPainter &painter, const QRectF &bounds, const QPalette &palette)
{
    const PaneColors colors(palette);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(colors.border);
    painter.setBrush(colors.surface);
    painter.drawRoundedRect(bounds.adjusted(0.5, 0.5, -0.5, -0.5), 14, 14);
    painter.restore();
}
