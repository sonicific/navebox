/* $Id$ */
/** @file
 * VBox Qt GUI - Scalable glass icon artwork.
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
#include <QHash>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QRegularExpression>

/* GUI includes: */
#include "UIIconPoolTheme.h"


/** @page pg_ui_icon_theme Application Icon Artwork
 *
 * Original vector artwork inspired by the simple silhouettes and restrained
 * depth of Fluent.  A 24-unit drawing grid, translucent gradients and a thin
 * rim replace the old bitmap shadows.  No background blur or external artwork
 * dependency is required.  The resource table is deliberately explicit: an
 * unknown resource or a user-provided icon always retains its original image.
 * Action modifiers and state marks are drawn separately from the base glyph.
 */

/** Semantic silhouettes used by the built-in icon set. */
enum UIIconGlyph
{
    UIICONGLYPH_MACHINE, UIICONGLYPH_ADD, UIICONGLYPH_REMOVE, UIICONGLYPH_CLOSE,
    UIICONGLYPH_PLAY, UIICONGLYPH_PAUSE, UIICONGLYPH_STOP, UIICONGLYPH_POWER,
    UIICONGLYPH_RESET, UIICONGLYPH_SAVE, UIICONGLYPH_SETTINGS, UIICONGLYPH_FOLDER,
    UIICONGLYPH_FILE, UIICONGLYPH_COPY, UIICONGLYPH_CUT, UIICONGLYPH_PASTE,
    UIICONGLYPH_DISK, UIICONGLYPH_OPTICAL, UIICONGLYPH_FLOPPY, UIICONGLYPH_CHIP,
    UIICONGLYPH_NETWORK, UIICONGLYPH_CLOUD, UIICONGLYPH_USB, UIICONGLYPH_DISPLAY,
    UIICONGLYPH_SOUND, UIICONGLYPH_MICROPHONE, UIICONGLYPH_KEYBOARD, UIICONGLYPH_MOUSE,
    UIICONGLYPH_SNAPSHOT, UIICONGLYPH_SEARCH, UIICONGLYPH_INFO, UIICONGLYPH_HELP,
    UIICONGLYPH_WARNING, UIICONGLYPH_ERROR, UIICONGLYPH_CHECK, UIICONGLYPH_DOWN,
    UIICONGLYPH_UP, UIICONGLYPH_LEFT, UIICONGLYPH_RIGHT, UIICONGLYPH_IMPORT,
    UIICONGLYPH_EXPORT, UIICONGLYPH_REFRESH, UIICONGLYPH_HOME, UIICONGLYPH_EXPAND,
    UIICONGLYPH_COLLAPSE, UIICONGLYPH_LINK, UIICONGLYPH_CLIPBOARD, UIICONGLYPH_CAMERA,
    UIICONGLYPH_FILM, UIICONGLYPH_CHART, UIICONGLYPH_GRID, UIICONGLYPH_LIST,
    UIICONGLYPH_PIN, UIICONGLYPH_STAR, UIICONGLYPH_BOOKMARK, UIICONGLYPH_FILTER,
    UIICONGLYPH_FONT, UIICONGLYPH_TERMINAL, UIICONGLYPH_PACKAGE, UIICONGLYPH_TOOLS,
    UIICONGLYPH_GROUP, UIICONGLYPH_GLOBE, UIICONGLYPH_PORT, UIICONGLYPH_MEMORY,
    UIICONGLYPH_SIZE, UIICONGLYPH_BELL, UIICONGLYPH_TRASH, UIICONGLYPH_SHIELD,
    UIICONGLYPH_MOVE, UIICONGLYPH_RENAME, UIICONGLYPH_MENU, UIICONGLYPH_EYE
};

/** Action/state modifiers, kept distinct from the underlying object. */
enum UIIconBadge
{
    UIICONBADGE_NONE, UIICONBADGE_ADD, UIICONBADGE_REMOVE, UIICONBADGE_EDIT,
    UIICONBADGE_CHECK, UIICONBADGE_OFF, UIICONBADGE_UP, UIICONBADGE_DOWN,
    UIICONBADGE_LEFT, UIICONBADGE_RIGHT, UIICONBADGE_WARNING, UIICONBADGE_ERROR,
    UIICONBADGE_PAUSE, UIICONBADGE_LINK, UIICONBADGE_SEARCH
};

/** Artwork assigned to one resource stem. */
struct UIIconArtwork
{
    UIIconGlyph enmGlyph; /**< Base silhouette. */
    UIIconBadge enmBadge; /**< Optional action or state. */
};

/* Explicit resource names live separately from the rendering code. */
#include "UIIconPoolThemeData.h"


/** Returns a restrained semantic color, independent of the current palette. */
static QColor uiIconColor(UIIconGlyph enmGlyph)
{
    switch (enmGlyph)
    {
        case UIICONGLYPH_PLAY: case UIICONGLYPH_CHECK: case UIICONGLYPH_ADD:
            return QColor(31, 153, 116);
        case UIICONGLYPH_POWER: case UIICONGLYPH_ERROR: case UIICONGLYPH_TRASH:
        case UIICONGLYPH_CLOSE: case UIICONGLYPH_STOP:
            return QColor(204, 76, 98);
        case UIICONGLYPH_WARNING: case UIICONGLYPH_PAUSE: case UIICONGLYPH_FOLDER:
        case UIICONGLYPH_STAR: case UIICONGLYPH_PACKAGE:
            return QColor(193, 134, 38);
        case UIICONGLYPH_SETTINGS: case UIICONGLYPH_TOOLS: case UIICONGLYPH_SNAPSHOT:
        case UIICONGLYPH_CHIP: case UIICONGLYPH_MEMORY:
            return QColor(34, 112, 218);
        default:
            return QColor(0, 142, 235);
    }
}


/** Paints a silhouette on the shared 24-unit grid. */
static void uiIconDrawGlyph(QPainter &painter, UIIconGlyph enmGlyph)
{
    QPainterPath path;
    switch (enmGlyph)
    {
        case UIICONGLYPH_MACHINE: case UIICONGLYPH_DISPLAY:
            painter.drawRoundedRect(QRectF(3, 4, 18, 13), 2, 2);
            painter.drawLine(QPointF(12, 17), QPointF(12, 21));
            painter.drawLine(QPointF(8, 21), QPointF(16, 21));
            if (enmGlyph == UIICONGLYPH_MACHINE)
                painter.drawRoundedRect(QRectF(6, 7, 7, 6), 1, 1);
            break;
        case UIICONGLYPH_ADD: case UIICONGLYPH_REMOVE: case UIICONGLYPH_CLOSE:
            painter.setBrush(Qt::NoBrush);
            if (enmGlyph == UIICONGLYPH_CLOSE)
            {
                painter.drawLine(QPointF(6, 6), QPointF(18, 18));
                painter.drawLine(QPointF(18, 6), QPointF(6, 18));
            }
            else
            {
                painter.drawLine(QPointF(5, 12), QPointF(19, 12));
                if (enmGlyph == UIICONGLYPH_ADD)
                    painter.drawLine(QPointF(12, 5), QPointF(12, 19));
            }
            break;
        case UIICONGLYPH_PLAY:
            path.moveTo(7, 4); path.lineTo(20, 12); path.lineTo(7, 20); path.closeSubpath();
            painter.drawPath(path);
            break;
        case UIICONGLYPH_PAUSE:
            painter.drawRoundedRect(QRectF(5, 4, 4, 16), 1, 1);
            painter.drawRoundedRect(QRectF(15, 4, 4, 16), 1, 1);
            break;
        case UIICONGLYPH_STOP:
            painter.drawRoundedRect(QRectF(5, 5, 14, 14), 3, 3);
            break;
        case UIICONGLYPH_POWER:
            painter.setBrush(Qt::NoBrush);
            painter.drawArc(QRectF(4, 4, 16, 16), 135 * 16, 270 * 16);
            painter.drawLine(QPointF(12, 3), QPointF(12, 11));
            break;
        case UIICONGLYPH_RESET: case UIICONGLYPH_REFRESH:
            painter.setBrush(Qt::NoBrush);
            painter.drawArc(QRectF(4, 4, 16, 16), 45 * 16, 290 * 16);
            painter.drawPolyline(QPolygonF() << QPointF(13, 4) << QPointF(18, 4) << QPointF(18, 9));
            break;
        case UIICONGLYPH_SAVE: case UIICONGLYPH_FLOPPY:
            painter.drawRoundedRect(QRectF(4, 3, 16, 18), 2, 2);
            painter.drawRect(QRectF(8, 3, 8, 6));
            painter.drawRoundedRect(QRectF(8, 13, 8, 8), 1, 1);
            break;
        case UIICONGLYPH_SETTINGS:
            path.addEllipse(QRectF(6, 6, 12, 12));
            for (int i = 0; i < 8; ++i)
            {
                QTransform transform;
                transform.translate(12, 12); transform.rotate(i * 45);
                QPainterPath tooth; tooth.addRoundedRect(QRectF(-2, -9, 4, 5), 1, 1);
                path = path.united(transform.map(tooth));
            }
            path.addEllipse(QRectF(9, 9, 6, 6));
            path.setFillRule(Qt::OddEvenFill);
            painter.drawPath(path);
            break;
        case UIICONGLYPH_FOLDER:
            path.moveTo(3, 7); path.lineTo(3, 5); path.quadTo(3, 3, 5, 3);
            path.lineTo(10, 3); path.lineTo(12, 6); path.lineTo(19, 6);
            path.quadTo(21, 6, 21, 8); path.lineTo(21, 19);
            path.quadTo(21, 21, 19, 21); path.lineTo(5, 21);
            path.quadTo(3, 21, 3, 19); path.closeSubpath();
            painter.drawPath(path); painter.drawLine(QPointF(3, 9), QPointF(21, 9));
            break;
        case UIICONGLYPH_FILE: case UIICONGLYPH_CLIPBOARD: case UIICONGLYPH_PASTE:
        case UIICONGLYPH_LIST: case UIICONGLYPH_RENAME:
            painter.drawRoundedRect(QRectF(5, 3, 14, 18), 2, 2);
            painter.drawLine(QPointF(8, 10), QPointF(16, 10));
            painter.drawLine(QPointF(8, 14), QPointF(16, 14));
            painter.drawLine(QPointF(8, 18), QPointF(13, 18));
            if (enmGlyph == UIICONGLYPH_CLIPBOARD || enmGlyph == UIICONGLYPH_PASTE)
                painter.drawRoundedRect(QRectF(9, 2, 6, 4), 1, 1);
            break;
        case UIICONGLYPH_COPY: case UIICONGLYPH_GROUP:
            painter.drawRoundedRect(QRectF(3, 3, 13, 14), 2, 2);
            painter.drawRoundedRect(QRectF(8, 8, 13, 13), 2, 2);
            break;
        case UIICONGLYPH_CUT:
            painter.drawEllipse(QRectF(3, 14, 6, 6)); painter.drawEllipse(QRectF(15, 14, 6, 6));
            painter.drawLine(QPointF(7, 15), QPointF(18, 3));
            painter.drawLine(QPointF(17, 15), QPointF(6, 3));
            break;
        case UIICONGLYPH_DISK:
            painter.drawRoundedRect(QRectF(4, 3, 16, 18), 3, 3);
            painter.drawEllipse(QRectF(7, 6, 10, 10)); painter.drawEllipse(QRectF(11, 10, 2, 2));
            painter.drawLine(QPointF(13, 15), QPointF(18, 18));
            break;
        case UIICONGLYPH_OPTICAL:
            path.addEllipse(QRectF(3, 3, 18, 18)); path.addEllipse(QRectF(9, 9, 6, 6));
            painter.drawPath(path);
            break;
        case UIICONGLYPH_CHIP: case UIICONGLYPH_MEMORY:
            painter.drawRoundedRect(QRectF(6, 6, 12, 12), 2, 2);
            for (int i = 8; i <= 16; i += 4)
            {
                painter.drawLine(QPointF(i, 3), QPointF(i, 6));
                painter.drawLine(QPointF(i, 18), QPointF(i, 21));
                painter.drawLine(QPointF(3, i), QPointF(6, i));
                painter.drawLine(QPointF(18, i), QPointF(21, i));
            }
            painter.drawRect(QRectF(9, 9, 6, 6));
            break;
        case UIICONGLYPH_NETWORK:
            painter.drawLine(QPointF(12, 8), QPointF(12, 14));
            painter.drawLine(QPointF(5, 14), QPointF(19, 14));
            painter.drawLine(QPointF(5, 14), QPointF(5, 17));
            painter.drawLine(QPointF(19, 14), QPointF(19, 17));
            painter.drawRoundedRect(QRectF(8, 3, 8, 6), 1, 1);
            painter.drawRoundedRect(QRectF(2, 17, 6, 4), 1, 1);
            painter.drawRoundedRect(QRectF(16, 17, 6, 4), 1, 1);
            break;
        case UIICONGLYPH_CLOUD:
            path.moveTo(6, 19); path.cubicTo(0, 19, 1, 10, 6, 10);
            path.cubicTo(6, 2, 17, 2, 18, 10); path.cubicTo(24, 10, 24, 19, 18, 19);
            path.closeSubpath(); painter.drawPath(path);
            break;
        case UIICONGLYPH_USB:
            painter.drawRoundedRect(QRectF(8, 9, 8, 12), 2, 2);
            painter.drawRect(QRectF(9, 3, 6, 6));
            painter.drawLine(QPointF(11, 5), QPointF(11, 7));
            painter.drawLine(QPointF(13, 5), QPointF(13, 7));
            break;
        case UIICONGLYPH_SOUND:
            path.moveTo(3, 9); path.lineTo(7, 9); path.lineTo(12, 5);
            path.lineTo(12, 19); path.lineTo(7, 15); path.lineTo(3, 15); path.closeSubpath();
            painter.drawPath(path); painter.setBrush(Qt::NoBrush);
            painter.drawArc(QRectF(9, 5, 12, 14), -60 * 16, 120 * 16);
            break;
        case UIICONGLYPH_MICROPHONE:
            painter.drawRoundedRect(QRectF(9, 3, 6, 12), 3, 3);
            painter.setBrush(Qt::NoBrush); painter.drawArc(QRectF(6, 7, 12, 12), 180 * 16, 180 * 16);
            painter.drawLine(QPointF(12, 19), QPointF(12, 22));
            break;
        case UIICONGLYPH_KEYBOARD:
            painter.drawRoundedRect(QRectF(2, 6, 20, 13), 2, 2);
            for (int i = 6; i <= 18; i += 4)
            {
                painter.drawPoint(QPointF(i, 10)); painter.drawPoint(QPointF(i, 13));
            }
            painter.drawLine(QPointF(8, 16), QPointF(16, 16));
            break;
        case UIICONGLYPH_MOUSE:
            painter.drawRoundedRect(QRectF(6, 3, 12, 18), 6, 6);
            painter.drawLine(QPointF(12, 4), QPointF(12, 10));
            break;
        case UIICONGLYPH_SNAPSHOT: case UIICONGLYPH_CAMERA:
            painter.drawRoundedRect(QRectF(3, 7, 18, 13), 3, 3);
            painter.drawRoundedRect(QRectF(8, 3, 8, 4), 1, 1);
            painter.drawEllipse(QRectF(8, 10, 8, 7));
            break;
        case UIICONGLYPH_SEARCH:
            painter.setBrush(Qt::NoBrush); painter.drawEllipse(QRectF(3, 3, 13, 13));
            painter.drawLine(QPointF(15, 15), QPointF(21, 21));
            break;
        case UIICONGLYPH_INFO: case UIICONGLYPH_HELP: case UIICONGLYPH_ERROR:
            painter.drawEllipse(QRectF(3, 3, 18, 18)); painter.setPen(QPen(Qt::white, 1.8, Qt::SolidLine, Qt::RoundCap));
            painter.setBrush(Qt::NoBrush);
            if (enmGlyph == UIICONGLYPH_HELP)
            {
                path.moveTo(9, 9); path.cubicTo(9, 5, 17, 6, 15, 10); path.lineTo(12, 13);
                painter.drawPath(path); painter.drawPoint(QPointF(12, 17));
            }
            else if (enmGlyph == UIICONGLYPH_ERROR)
            {
                painter.drawLine(QPointF(8, 8), QPointF(16, 16));
                painter.drawLine(QPointF(8, 16), QPointF(16, 8));
            }
            else
            {
                painter.drawLine(QPointF(12, 10), QPointF(12, 16));
                painter.drawPoint(QPointF(12, 7));
            }
            break;
        case UIICONGLYPH_WARNING:
            path.moveTo(12, 3); path.lineTo(22, 21); path.lineTo(2, 21); path.closeSubpath();
            painter.drawPath(path); painter.drawLine(QPointF(12, 9), QPointF(12, 14));
            painter.drawPoint(QPointF(12, 18));
            break;
        case UIICONGLYPH_CHECK:
            painter.setBrush(Qt::NoBrush);
            painter.drawPolyline(QPolygonF() << QPointF(4, 12) << QPointF(9, 17) << QPointF(20, 6));
            break;
        case UIICONGLYPH_DOWN: case UIICONGLYPH_UP: case UIICONGLYPH_LEFT: case UIICONGLYPH_RIGHT:
        case UIICONGLYPH_IMPORT: case UIICONGLYPH_EXPORT:
            painter.translate(12, 12);
            if (enmGlyph == UIICONGLYPH_UP || enmGlyph == UIICONGLYPH_EXPORT) painter.rotate(180);
            if (enmGlyph == UIICONGLYPH_LEFT) painter.rotate(90);
            if (enmGlyph == UIICONGLYPH_RIGHT) painter.rotate(-90);
            painter.drawLine(QPointF(0, -8), QPointF(0, 7));
            painter.drawPolyline(QPolygonF() << QPointF(-6, 1) << QPointF(0, 7) << QPointF(6, 1));
            if (enmGlyph == UIICONGLYPH_IMPORT || enmGlyph == UIICONGLYPH_EXPORT)
                painter.drawLine(QPointF(-8, 10), QPointF(8, 10));
            break;
        case UIICONGLYPH_HOME:
            path.moveTo(3, 10); path.lineTo(12, 3); path.lineTo(21, 10);
            path.lineTo(19, 10); path.lineTo(19, 21); path.lineTo(5, 21); path.lineTo(5, 10);
            path.closeSubpath(); painter.drawPath(path); painter.drawRect(QRectF(10, 14, 4, 7));
            break;
        case UIICONGLYPH_EXPAND: case UIICONGLYPH_COLLAPSE: case UIICONGLYPH_SIZE:
            painter.setBrush(Qt::NoBrush);
            painter.drawPolyline(QPolygonF() << QPointF(3, 9) << QPointF(3, 3) << QPointF(9, 3));
            painter.drawPolyline(QPolygonF() << QPointF(15, 21) << QPointF(21, 21) << QPointF(21, 15));
            painter.drawLine(QPointF(4, 4), QPointF(9, 9));
            painter.drawLine(QPointF(15, 15), QPointF(20, 20));
            if (enmGlyph == UIICONGLYPH_COLLAPSE)
            {
                painter.drawLine(QPointF(3, 9), QPointF(9, 9));
                painter.drawLine(QPointF(15, 15), QPointF(21, 15));
            }
            break;
        case UIICONGLYPH_LINK:
            painter.translate(12, 12); painter.rotate(-35);
            painter.drawRoundedRect(QRectF(-10, -4, 12, 8), 4, 4);
            painter.drawRoundedRect(QRectF(-2, -4, 12, 8), 4, 4);
            break;
        case UIICONGLYPH_FILM:
            painter.drawRoundedRect(QRectF(3, 4, 18, 16), 2, 2);
            painter.drawLine(QPointF(7, 4), QPointF(7, 20));
            painter.drawLine(QPointF(17, 4), QPointF(17, 20));
            for (int i = 8; i <= 16; i += 4)
            {
                painter.drawLine(QPointF(3, i), QPointF(7, i));
                painter.drawLine(QPointF(17, i), QPointF(21, i));
            }
            break;
        case UIICONGLYPH_CHART:
            painter.drawRoundedRect(QRectF(3, 13, 4, 8), 1, 1);
            painter.drawRoundedRect(QRectF(10, 8, 4, 13), 1, 1);
            painter.drawRoundedRect(QRectF(17, 3, 4, 18), 1, 1);
            break;
        case UIICONGLYPH_GRID:
            for (int i = 0; i < 4; ++i)
                painter.drawRoundedRect(QRectF(3 + i % 2 * 10, 3 + i / 2 * 10, 7, 7), 2, 2);
            break;
        case UIICONGLYPH_PIN:
            path.moveTo(8, 3); path.lineTo(18, 3); path.lineTo(16, 11); path.lineTo(20, 15);
            path.lineTo(5, 15); path.lineTo(9, 11); path.closeSubpath(); painter.drawPath(path);
            painter.drawLine(QPointF(12, 15), QPointF(12, 22));
            break;
        case UIICONGLYPH_STAR:
            path.moveTo(12, 2); path.lineTo(15, 8); path.lineTo(22, 10); path.lineTo(17, 15);
            path.lineTo(18, 22); path.lineTo(12, 18); path.lineTo(6, 22); path.lineTo(7, 15);
            path.lineTo(2, 10); path.lineTo(9, 8); path.closeSubpath(); painter.drawPath(path);
            break;
        case UIICONGLYPH_BOOKMARK:
            path.moveTo(6, 3); path.lineTo(18, 3); path.lineTo(18, 21); path.lineTo(12, 16);
            path.lineTo(6, 21); path.closeSubpath(); painter.drawPath(path);
            break;
        case UIICONGLYPH_FILTER:
            path.moveTo(3, 4); path.lineTo(21, 4); path.lineTo(14, 12); path.lineTo(14, 20);
            path.lineTo(10, 22); path.lineTo(10, 12); path.closeSubpath(); painter.drawPath(path);
            break;
        case UIICONGLYPH_FONT:
            painter.setBrush(Qt::NoBrush);
            painter.drawPolyline(QPolygonF() << QPointF(4, 21) << QPointF(12, 3) << QPointF(20, 21));
            painter.drawLine(QPointF(7, 15), QPointF(17, 15));
            break;
        case UIICONGLYPH_TERMINAL:
            painter.drawRoundedRect(QRectF(3, 4, 18, 16), 2, 2);
            painter.drawPolyline(QPolygonF() << QPointF(7, 8) << QPointF(11, 12) << QPointF(7, 16));
            painter.drawLine(QPointF(13, 16), QPointF(17, 16));
            break;
        case UIICONGLYPH_PACKAGE:
            path.moveTo(3, 7); path.lineTo(12, 3); path.lineTo(21, 7); path.lineTo(21, 18);
            path.lineTo(12, 22); path.lineTo(3, 18); path.closeSubpath(); painter.drawPath(path);
            painter.drawPolyline(QPolygonF() << QPointF(3, 7) << QPointF(12, 11) << QPointF(21, 7));
            painter.drawLine(QPointF(12, 11), QPointF(12, 22));
            break;
        case UIICONGLYPH_TOOLS:
            path.moveTo(4, 20); path.lineTo(14, 10); path.cubicTo(10, 3, 17, 1, 19, 3);
            path.lineTo(16, 6); path.lineTo(18, 8); path.lineTo(21, 5);
            path.cubicTo(23, 11, 19, 14, 15, 12); path.lineTo(6, 22); path.closeSubpath();
            painter.drawPath(path);
            break;
        case UIICONGLYPH_GLOBE:
            painter.drawEllipse(QRectF(3, 3, 18, 18)); painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(QRectF(8, 3, 8, 18)); painter.drawLine(QPointF(3, 12), QPointF(21, 12));
            break;
        case UIICONGLYPH_PORT:
            painter.drawRoundedRect(QRectF(3, 6, 18, 12), 3, 3);
            for (int i = 7; i <= 17; i += 5) painter.drawPoint(QPointF(i, 12));
            break;
        case UIICONGLYPH_BELL:
            path.moveTo(4, 18); path.lineTo(6, 15); path.lineTo(6, 9);
            path.cubicTo(6, 1, 18, 1, 18, 9); path.lineTo(18, 15); path.lineTo(20, 18);
            path.closeSubpath(); painter.drawPath(path); painter.drawLine(QPointF(10, 21), QPointF(14, 21));
            break;
        case UIICONGLYPH_TRASH:
            painter.drawRoundedRect(QRectF(6, 7, 12, 14), 2, 2);
            painter.drawLine(QPointF(4, 6), QPointF(20, 6));
            painter.drawLine(QPointF(9, 3), QPointF(15, 3));
            painter.drawLine(QPointF(10, 10), QPointF(10, 17));
            painter.drawLine(QPointF(14, 10), QPointF(14, 17));
            break;
        case UIICONGLYPH_SHIELD:
            path.moveTo(12, 2); path.lineTo(21, 6); path.cubicTo(21, 15, 18, 19, 12, 22);
            path.cubicTo(6, 19, 3, 15, 3, 6); path.closeSubpath(); painter.drawPath(path);
            break;
        case UIICONGLYPH_MOVE:
            painter.drawLine(QPointF(3, 12), QPointF(21, 12));
            painter.drawLine(QPointF(12, 3), QPointF(12, 21));
            painter.drawPolyline(QPolygonF() << QPointF(8, 7) << QPointF(12, 3) << QPointF(16, 7));
            painter.drawPolyline(QPolygonF() << QPointF(8, 17) << QPointF(12, 21) << QPointF(16, 17));
            break;
        case UIICONGLYPH_MENU:
            painter.drawLine(QPointF(5, 6), QPointF(19, 6));
            painter.drawLine(QPointF(5, 12), QPointF(19, 12));
            painter.drawLine(QPointF(5, 18), QPointF(19, 18));
            break;
        case UIICONGLYPH_EYE:
            path.moveTo(2, 12); path.quadTo(12, -2, 22, 12);
            path.quadTo(12, 26, 2, 12); path.closeSubpath();
            painter.drawPath(path); painter.drawEllipse(QRectF(9, 9, 6, 6));
            break;
    }
}


/** Draws a readable modifier at the bottom right of a base silhouette. */
static void uiIconDrawBadge(QPainter &painter, UIIconBadge enmBadge, bool fDisabled)
{
    if (enmBadge == UIICONBADGE_NONE)
        return;
    QColor color = enmBadge == UIICONBADGE_WARNING ? QColor(166, 110, 24)
                 : enmBadge == UIICONBADGE_ERROR || enmBadge == UIICONBADGE_OFF ? QColor(184, 65, 86)
                 : QColor(44, 118, 157);
    if (fDisabled) color = QColor(128, 139, 151);
    painter.setPen(QPen(QColor(255, 255, 255, 225), 1.2));
    painter.setBrush(color);
    painter.drawEllipse(QRectF(12, 12, 11, 11));
    painter.setPen(QPen(Qt::white, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    switch (enmBadge)
    {
        case UIICONBADGE_ADD:
            painter.drawLine(QPointF(17.5, 14.5), QPointF(17.5, 20.5));
            painter.drawLine(QPointF(14.5, 17.5), QPointF(20.5, 17.5));
            break;
        case UIICONBADGE_REMOVE:
            painter.drawLine(QPointF(14.5, 17.5), QPointF(20.5, 17.5)); break;
        case UIICONBADGE_CHECK:
            painter.drawPolyline(QPolygonF() << QPointF(14.5, 17.5) << QPointF(16.5, 19.5) << QPointF(20.5, 15.5)); break;
        case UIICONBADGE_WARNING: case UIICONBADGE_ERROR:
            painter.drawLine(QPointF(17.5, 14.5), QPointF(17.5, 17.5));
            painter.drawPoint(QPointF(17.5, 20)); break;
        case UIICONBADGE_PAUSE:
            painter.drawLine(QPointF(16, 15), QPointF(16, 20));
            painter.drawLine(QPointF(19, 15), QPointF(19, 20)); break;
        case UIICONBADGE_OFF:
            painter.drawLine(QPointF(15, 20), QPointF(20, 15)); break;
        case UIICONBADGE_EDIT:
            painter.drawLine(QPointF(15, 20), QPointF(20, 15));
            painter.drawPoint(QPointF(15, 20.5)); break;
        case UIICONBADGE_SEARCH:
            painter.setBrush(Qt::NoBrush); painter.drawEllipse(QRectF(14.5, 14.5, 4, 4));
            painter.drawLine(QPointF(18, 18), QPointF(20, 20)); break;
        case UIICONBADGE_LINK:
            painter.drawLine(QPointF(15, 19), QPointF(20, 16)); break;
        case UIICONBADGE_UP: case UIICONBADGE_DOWN: case UIICONBADGE_LEFT: case UIICONBADGE_RIGHT:
            painter.translate(17.5, 17.5);
            if (enmBadge == UIICONBADGE_UP) painter.rotate(180);
            if (enmBadge == UIICONBADGE_LEFT) painter.rotate(90);
            if (enmBadge == UIICONBADGE_RIGHT) painter.rotate(-90);
            painter.drawLine(QPointF(0, -3), QPointF(0, 3));
            painter.drawPolyline(QPolygonF() << QPointF(-2, 1) << QPointF(0, 3) << QPointF(2, 1));
            break;
        case UIICONBADGE_NONE: break;
    }
}


/* static */
QPixmap UIIconPoolTheme::pixmap(const QString &strName, int iScale /* = 1 */)
{
    /* Restrict substitutions to known built-in small application icons. */
    static const QRegularExpression reName(QStringLiteral("^:/([a-z0-9_]+)_([0-9]+)px\\.png$"));
    const QRegularExpressionMatch match = reName.match(strName);
    if (!match.hasMatch() || iScale < 1 || iScale > 4)
        return QPixmap();
    const int iSize = match.captured(2).toInt();
    if (iSize < 10 || iSize > 32)
        return QPixmap();
    QString strStem = match.captured(1);
    const bool fDisabled = strStem.contains(QStringLiteral("_disabled"));
    strStem.remove(QStringLiteral("_disabled"));
    const bool fWarning = strStem.endsWith(QStringLiteral("_warning"));
    if (fWarning) strStem.chop(8);
    const QHash<QString, UIIconArtwork>::const_iterator it = uiIconArtwork().constFind(strStem);
    if (it == uiIconArtwork().constEnd())
        return QPixmap();

    const QString strCacheKey = QStringLiteral("vbox-glass:%1:%2").arg(strName).arg(iScale);
    QPixmap pixmap;
    if (QPixmapCache::find(strCacheKey, &pixmap))
        return pixmap;
    pixmap = QPixmap(iSize * iScale, iSize * iScale);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(iSize * iScale / 24.0, iSize * iScale / 24.0);
    const QColor color = fDisabled ? QColor(137, 148, 161) : uiIconColor(it->enmGlyph);
    QLinearGradient gradient(4, 2, 19, 22);
    gradient.setColorAt(0, color.lighter(180));
    gradient.setColorAt(0.45, color.lighter(125));
    gradient.setColorAt(1, color);
    painter.setOpacity(fDisabled ? 0.5 : 1.0);
    painter.setPen(QPen(color.darker(120), 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(gradient);
    painter.save();
    uiIconDrawGlyph(painter, it->enmGlyph);
    painter.restore();
    painter.save();
    uiIconDrawBadge(painter, fWarning ? UIICONBADGE_WARNING : it->enmBadge, fDisabled);
    painter.restore();
    /* Host-key capture, press and insertion are independent runtime states. */
    if (strStem.startsWith(QStringLiteral("hostkey_")))
    {
        painter.setPen(QPen(Qt::white, 0.8));
        if (strStem.contains(QStringLiteral("_pressed")))
        {
            painter.setBrush(QColor(193, 134, 38));
            painter.drawRoundedRect(QRectF(1, 2, 7, 4), 1, 1);
        }
        if (strStem.contains(QStringLiteral("_checked")))
        {
            painter.setBrush(QColor(31, 153, 116));
            painter.drawEllipse(QRectF(17, 1, 5, 5));
        }
    }
    painter.end();
    QPixmapCache::insert(strCacheKey, pixmap);
    return pixmap;
}
