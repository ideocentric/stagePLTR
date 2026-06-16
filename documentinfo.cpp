/*
 * stagePLTR — stage plots and tech riders for bands.
 * Copyright (C) 2026 Matt Comeione
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "documentinfo.h"

#include <QFont>
#include <QImage>
#include <QJsonObject>
#include <QPainter>
#include <QRectF>
#include <QSvgRenderer>

QJsonObject DocumentInfo::toJson() const
{
    QJsonObject obj;
    if (!bandName.isEmpty())
        obj[QStringLiteral("bandName")] = bandName;
    if (date.isValid())
        obj[QStringLiteral("date")] = date.toString(Qt::ISODate);
    if (!logoData.isEmpty()) {
        obj[QStringLiteral("logo")] = QString::fromLatin1(logoData.toBase64());
        obj[QStringLiteral("logoFormat")] = logoFormat;
    }
    return obj;
}

DocumentInfo DocumentInfo::fromJson(const QJsonObject &obj)
{
    DocumentInfo info;
    info.bandName = obj.value(QStringLiteral("bandName")).toString();
    const QString dateStr = obj.value(QStringLiteral("date")).toString();
    if (!dateStr.isEmpty())
        info.date = QDate::fromString(dateStr, Qt::ISODate);
    const QString logo = obj.value(QStringLiteral("logo")).toString();
    if (!logo.isEmpty()) {
        info.logoData = QByteArray::fromBase64(logo.toLatin1());
        info.logoFormat = obj.value(QStringLiteral("logoFormat")).toString();
    }
    return info;
}

namespace {

// Scale `src` to fit within `bound`, left-aligned and vertically centred.
QRectF fitLeft(const QSizeF &src, const QRectF &bound)
{
    if (src.width() <= 0 || src.height() <= 0)
        return QRectF(bound.topLeft(), QSizeF(0, 0));
    const qreal s = qMin(bound.width() / src.width(), bound.height() / src.height());
    const qreal w = src.width() * s;
    const qreal h = src.height() * s;
    return QRectF(bound.left(), bound.top() + (bound.height() - h) / 2.0, w, h);
}

void drawLogo(QPainter &p, const QRectF &bound, const DocumentInfo &info)
{
    if (info.logoFormat.compare(QLatin1String("svg"), Qt::CaseInsensitive) == 0) {
        QSvgRenderer renderer(info.logoData);
        if (!renderer.isValid())
            return;
        QSizeF src = renderer.defaultSize();
        if (src.isEmpty())
            src = QSizeF(1, 1);
        renderer.render(&p, fitLeft(src, bound));
    } else {
        const QImage img = QImage::fromData(info.logoData);
        if (img.isNull())
            return;
        p.drawImage(fitLeft(QSizeF(img.size()), bound), img);
    }
}

} // namespace

void drawTitleBlock(QPainter &p, const QRectF &rect, const DocumentInfo &info,
                    const QString &section)
{
    if (!info.hasContent() && !info.date.isValid() && section.isEmpty())
        return;

    p.save();
    const qreal H = rect.height();

    // ---- Left: logo (preferred) or band name --------------------------------
    if (info.hasLogo()) {
        drawLogo(p, QRectF(rect.left(), rect.top(), rect.width() * 0.55, H), info);
    } else if (!info.bandName.isEmpty()) {
        QFont nameFont = p.font();
        nameFont.setBold(true);
        nameFont.setPixelSize(qMax(1, int(H * 0.5)));
        p.setFont(nameFont);
        p.setPen(QColor(0x1a, 0x1a, 0x1a));
        p.drawText(QRectF(rect.left(), rect.top(), rect.width() * 0.6, H),
                   Qt::AlignLeft | Qt::AlignVCenter, info.bandName);
    }

    // ---- Right: section label (top) and date (below), right-aligned ---------
    const qreal rightW = rect.width() * 0.4;
    const QRectF rightRect(rect.right() - rightW, rect.top(), rightW, H);

    if (!section.isEmpty()) {
        QFont secFont = p.font();
        secFont.setBold(true);
        secFont.setPixelSize(qMax(1, int(H * 0.34)));
        p.setFont(secFont);
        p.setPen(QColor(0x33, 0x33, 0x33));
        p.drawText(QRectF(rightRect.left(), rightRect.top(), rightW, H * 0.5),
                   Qt::AlignRight | Qt::AlignBottom, section);
    }
    if (info.date.isValid()) {
        QFont dateFont = p.font();
        dateFont.setBold(false);
        dateFont.setPixelSize(qMax(1, int(H * 0.28)));
        p.setFont(dateFont);
        p.setPen(QColor(0x66, 0x66, 0x66));
        const QString text = info.date.toString(QStringLiteral("MMMM d, yyyy"));
        const QRectF dateRect = section.isEmpty()
            ? rightRect
            : QRectF(rightRect.left(), rightRect.top() + H * 0.52, rightW, H * 0.48);
        p.drawText(dateRect,
                   section.isEmpty() ? (Qt::AlignRight | Qt::AlignVCenter)
                                     : (Qt::AlignRight | Qt::AlignTop),
                   text);
    }

    // ---- Divider beneath ----------------------------------------------------
    QPen divider(QColor(0xb5, 0xb8, 0xbb));
    divider.setWidthF(qMax(1.0, H * 0.015));
    p.setPen(divider);
    p.drawLine(rect.bottomLeft(), rect.bottomRight());

    p.restore();
}