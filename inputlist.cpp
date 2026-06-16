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

#include "inputlist.h"

#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QRectF>

namespace inputlist {

QString line(const InputListEntry &entry, bool showPhantom)
{
    QString text = QStringLiteral("%1. %2 %3 with %4 Level output")
                       .arg(entry.numbers, entry.signalWord, entry.connector, entry.level);
    if (showPhantom && entry.phantom)
        text += QStringLiteral(" +48V");
    return text;
}

int columnsFor(QPageLayout::Orientation orientation)
{
    return orientation == QPageLayout::Landscape ? 4 : 2;
}

int draw(QPainter *painter, const QRectF &area, const QList<InputListEntry> &entries,
         int columns, qreal rowH, bool showPhantom, int start)
{
    if (entries.isEmpty() || columns < 1 || rowH <= 0.0)
        return entries.size();

    const int rowsPerColumn = qMax(1, int(area.height() / rowH));
    const qreal colW = area.width() / columns;
    const qreal cellPad = colW * 0.04;

    if (painter) {
        QFont font = painter->font();
        font.setPixelSize(qMax(1, int(rowH * 0.62)));
        painter->setFont(font);
        painter->setPen(QColor(0x22, 0x22, 0x22));
    }
    const QFontMetricsF fm = painter ? QFontMetricsF(painter->font(), painter->device())
                                     : QFontMetricsF(QFont());

    int idx = start;
    for (int c = 0; c < columns && idx < entries.size(); ++c) {
        for (int r = 0; r < rowsPerColumn && idx < entries.size(); ++r, ++idx) {
            if (!painter)
                continue;
            const QRectF cell(area.left() + c * colW, area.top() + r * rowH,
                              colW - cellPad, rowH);
            const QString text =
                fm.elidedText(line(entries.at(idx), showPhantom), Qt::ElideRight,
                              cell.width());
            painter->drawText(cell, Qt::AlignLeft | Qt::AlignVCenter, text);
        }
    }
    return idx;
}

} // namespace inputlist