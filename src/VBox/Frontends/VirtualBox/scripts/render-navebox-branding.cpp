/* $Id$ */
/** @file
 * VBox Qt GUI - Render the Navebox vector identity into desktop and BIOS assets.
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
#include <QBuffer>
#include <QCoreApplication>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QMap>
#include <QPainter>
#include <QSvgRenderer>

/** @page pg_navebox_brand_assets Navebox Identity Assets
 *
 * Developer-only Qt Core/Gui/Svg utility.  Run with the SVG path and output
 * directory.  The SVG is the editable source; PNG, ICO and ICNS are exports.
 * The --boot mode renders an uncompressed 24-bit BMP for the legacy BIOS.
 * No image editing or SVG library is added to the application runtime.
 */

/** Returns the number of bytes written, or -1 if the file cannot be opened. */
static qint64 uiBrandWrite(const QString &strPath, const QByteArray &data)
{
    QFile file(strPath);
    if (!file.open(QIODevice::WriteOnly))
        return -1;
    return file.write(data);
}

/** Renders the identity and font-independent wordmark in the BIOS bitmap format. */
static int uiBrandRenderBootLogo(const QString &strIcon, const QString &strWordmark, const QString &strOutput)
{
    QSvgRenderer icon(strIcon);
    QSvgRenderer wordmark(strWordmark);
    if (!icon.isValid() || !wordmark.isValid())
        return 2;
    QImage image(560, 160, QImage::Format_RGB888);
    image.fill(Qt::black);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    icon.render(&painter, QRectF(0, 0, 160, 160));
    wordmark.render(&painter, QRectF(176, 38, 384, 84));
    painter.end();
    return image.save(strOutput, "BMP") ? 0 : 5;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc == 5 && qstrcmp(argv[1], "--boot") == 0)
        return uiBrandRenderBootLogo(QString::fromLocal8Bit(argv[2]), QString::fromLocal8Bit(argv[3]),
                                     QString::fromLocal8Bit(argv[4]));
    if (argc != 3)
        return 1;
    QSvgRenderer renderer(QString::fromLocal8Bit(argv[1]));
    if (!renderer.isValid())
        return 2;
    const QDir output(QString::fromLocal8Bit(argv[2]));
    if (!QDir().mkpath(output.absolutePath()))
        return 3;
    QMap<int, QByteArray> images;
    for (int iSize : {16, 20, 24, 32, 40, 48, 64, 70, 96, 128, 150, 192, 256, 512, 1024})
    {
        QImage image(iSize, iSize, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        renderer.render(&painter);
        painter.end();
        QByteArray data;
        QBuffer buffer(&data);
        if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG"))
            return 4;
        images.insert(iSize, data);
        if (uiBrandWrite(output.filePath(QStringLiteral("navebox-%1.png").arg(iSize)), data) != data.size())
            return 5;
    }
    /* Flat control buttons need their own arrows once native bevel painting is replaced. */
    for (bool fDark : {false, true})
        for (bool fUp : {false, true})
        {
            QImage image(24, 16, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(QPen(fDark ? QColor(237, 242, 250) : QColor(52, 65, 88),
                                2.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            if (fUp)
            {
                painter.translate(12, 8);
                painter.rotate(180);
                painter.translate(-12, -8);
            }
            painter.drawPolyline(QPolygonF() << QPointF(5, 5) << QPointF(12, 11) << QPointF(19, 5));
            painter.end();
            const QString strFile = QStringLiteral("chevron-%1-%2.png")
                .arg(fUp ? QStringLiteral("up") : QStringLiteral("down"),
                     fDark ? QStringLiteral("dark") : QStringLiteral("light"));
            if (!image.save(output.filePath(strFile)))
                return 8;
        }
    const QList<int> sizes = {16, 20, 24, 32, 40, 48, 64, 128, 256};
    QByteArray ico;
    QDataStream stream(&ico, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint16(0) << quint16(1) << quint16(sizes.size());
    quint32 offImage = 6 + quint32(sizes.size()) * 16;
    for (int iSize : sizes)
    {
        const quint8 bSize = iSize == 256 ? 0 : quint8(iSize);
        stream << bSize << bSize << quint8(0) << quint8(0) << quint16(1) << quint16(32)
               << quint32(images.value(iSize).size()) << offImage;
        offImage += quint32(images.value(iSize).size());
    }
    for (int iSize : sizes)
        ico.append(images.value(iSize));
    if (uiBrandWrite(output.filePath(QStringLiteral("navebox.ico")), ico) != ico.size())
        return 6;
    QByteArray icnsBody;
    for (const auto &entry : {qMakePair(128, "ic07"), qMakePair(256, "ic08"),
                              qMakePair(512, "ic09"), qMakePair(1024, "ic10")})
    {
        QByteArray chunk;
        QDataStream chunkStream(&chunk, QIODevice::WriteOnly);
        chunkStream.setByteOrder(QDataStream::BigEndian);
        chunkStream.writeRawData(entry.second, 4);
        chunkStream << quint32(images.value(entry.first).size() + 8);
        chunk.append(images.value(entry.first));
        icnsBody.append(chunk);
    }
    QByteArray icns;
    QDataStream macStream(&icns, QIODevice::WriteOnly);
    macStream.setByteOrder(QDataStream::BigEndian);
    macStream.writeRawData("icns", 4);
    macStream << quint32(icnsBody.size() + 8);
    icns.append(icnsBody);
    return uiBrandWrite(output.filePath(QStringLiteral("navebox.icns")), icns) == icns.size() ? 0 : 7;
}
