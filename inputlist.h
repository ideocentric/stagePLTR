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

#ifndef INPUTLIST_H
#define INPUTLIST_H

#include <QList>
#include <QPageLayout>
#include <QString>

class QPainter;
class QRectF;

// One condensed input-list line. A stereo output is a single entry spanning two
// channel numbers (e.g. "1–2"); a mono output spans one. Built by the scene
// alongside channel numbering so it matches the on-symbol badges.
struct InputListEntry
{
    QString numbers;     // "1" or "1–2"
    QString signalWord;  // "Mono" / "Stereo"
    QString connector;   // e.g. "XLR"
    QString level;       // e.g. "Line"
    bool phantom = false;
};

namespace inputlist {

// "1–2. Stereo XLR with Line Level output" (+ " +48V" when showPhantom and the
// entry carries phantom power).
QString line(const InputListEntry &entry, bool showPhantom);

// Column count for the condensed list: 2 portrait, 4 landscape.
int columnsFor(QPageLayout::Orientation orientation);

// Lay out entries [start, …) into `area`, filling each column top-to-bottom
// before moving right, at row height `rowH` (in the painter's own units). A
// null painter measures only. Returns the index just past the last entry that
// fit — equal to entries.size() when everything was placed.
int draw(QPainter *painter, const QRectF &area, const QList<InputListEntry> &entries,
         int columns, qreal rowH, bool showPhantom, int start = 0);

} // namespace inputlist

#endif // INPUTLIST_H