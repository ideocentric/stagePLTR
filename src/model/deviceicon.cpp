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

#include "deviceicon.h"

#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QSvgRenderer>

namespace {
constexpr int kIconPreviewPx = 128;  // raster size for the palette QIcon
}

DeviceIcon DeviceIcon::fromPath(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return DeviceIcon(file.readAll(), QFileInfo(path).suffix());
}

void DeviceIcon::render(QPainter *painter, const QRectF &rect) const
{
    if (!isValid() || !painter)
        return;
    if (isSvg()) {
        QSvgRenderer renderer(m_data);
        if (renderer.isValid())
            renderer.render(painter, rect);
    } else {
        const QImage image = QImage::fromData(m_data);
        if (!image.isNull())
            painter->drawImage(rect, image);
    }
}

QIcon DeviceIcon::toIcon() const
{
    if (!isValid())
        return {};
    if (isSvg()) {
        QSvgRenderer renderer(m_data);
        if (!renderer.isValid())
            return {};
        QPixmap pixmap(kIconPreviewPx, kIconPreviewPx);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        renderer.render(&painter, QRectF(pixmap.rect()));
        return QIcon(pixmap);
    }
    QPixmap pixmap;
    pixmap.loadFromData(m_data);
    return pixmap.isNull() ? QIcon() : QIcon(pixmap);
}