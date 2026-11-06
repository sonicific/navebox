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

#ifndef FEQT_INCLUDED_SRC_globals_UIAppearance_h
#define FEQT_INCLUDED_SRC_globals_UIAppearance_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <QPalette>
#include <QString>

class QPainter;
class QRectF;

/** Shared control appearance, independent of product identity and layout. */
class UIAppearance
{
public:
    /** Palette shared by Manager pages and sliding sheets. */
    struct PaneColors
    {
        /** Derives surface and interaction colors from @a palette. */
        explicit PaneColors(const QPalette &palette)
            : fLight(palette.color(QPalette::Window).lightness() >= 128
                     && palette.color(QPalette::WindowText).lightness() < 128)
            , backdrop(fLight ? QColor("#f3f6fc") : palette.color(QPalette::Window))
            , surface(fLight ? QColor("#fcfcfe") : palette.color(QPalette::Base))
            , border(fLight ? QColor("#e8edf6") : blend(surface, palette.color(QPalette::WindowText), 12))
            , divider(fLight ? QColor("#ebeff6") : blend(surface, palette.color(QPalette::WindowText), 8))
            , row(fLight ? QColor("#f3f5fb") : blend(surface, palette.color(QPalette::Highlight), 6))
            , hover(fLight ? QColor("#eaf0fb") : blend(surface, palette.color(QPalette::Highlight), 14))
            , pressed(fLight ? QColor("#e1eafb") : blend(surface, palette.color(QPalette::Highlight), 22))
            , text(fLight ? QColor("#0b1020") : palette.color(QPalette::WindowText))
            , secondary(fLight ? QColor("#3d4862") : blend(text, row, 23))
            , scrollThumb(fLight ? QColor("#9aa9be") : blend(row, text, 38))
            , scrollHover(fLight ? QColor("#798ca8") : blend(row, text, 55))
            , scrollPressed(fLight ? QColor("#657b9a") : blend(row, text, 68))
        {}

        const bool   fLight;    /**< Whether the light surfaces are in use. */
        const QColor backdrop; /**< Background around cards. */
        const QColor surface;  /**< Main card and sheet background. */
        const QColor border;   /**< Card outline. */
        const QColor divider;  /**< Internal separators. */
        const QColor row;      /**< Inset content and idle rows. */
        const QColor hover;    /**< Hover surface. */
        const QColor pressed;  /**< Pressed surface. */
        const QColor text;     /**< Primary text. */
        const QColor secondary;/**< Secondary text. */
        const QColor scrollThumb; /**< Resting scrollbar thumb. */
        const QColor scrollHover; /**< Hovered scrollbar thumb. */
        const QColor scrollPressed; /**< Scrollbar thumb during dragging. */

    private:
        /** Blends @a iPercent of @a tint into @a base. */
        static QColor blend(const QColor &base, const QColor &tint, int iPercent)
        {
            return QColor((base.red()   * (100 - iPercent) + tint.red()   * iPercent) / 100,
                          (base.green() * (100 - iPercent) + tint.green() * iPercent) / 100,
                          (base.blue()  * (100 - iPercent) + tint.blue()  * iPercent) / 100);
        }
    };

    /** Returns flat, palette-aware input and group-box styling. */
    static QString styleSheet(const QPalette &palette);

    /** Draws a rounded, opaque sheet in @a bounds. */
    static void paintSheet(QPainter &painter, const QRectF &bounds, const QPalette &palette);
};

#endif /* !FEQT_INCLUDED_SRC_globals_UIAppearance_h */
