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

#ifndef DOCUMENTINFO_H
#define DOCUMENTINFO_H

#include <QByteArray>
#include <QDate>
#include <QString>

class QJsonObject;
class QPainter;
class QRectF;

// Document-level metadata for the rider's title block. The logo image is
// embedded (raw bytes + format) so the .splot file stays self-contained.
struct DocumentInfo
{
    QString bandName;
    QDate date;
    QByteArray logoData;    // raw image bytes (empty = no logo)
    QString logoFormat;     // lowercased suffix: "svg", "png", "jpg", …

    bool hasLogo() const { return !logoData.isEmpty(); }
    bool hasContent() const { return !bandName.isEmpty() || hasLogo(); }

    QJsonObject toJson() const;
    static DocumentInfo fromJson(const QJsonObject &obj);
};

// Draws the title block into `rect`: the logo if present (replacing the band
// name), otherwise the band name; with the date and an optional section label
// (e.g. "Input List") right-aligned, and a divider beneath. Font sizes derive
// from rect height, so it works in both scene units and PDF device pixels.
void drawTitleBlock(QPainter &p, const QRectF &rect, const DocumentInfo &info,
                    const QString &section);

// Draws just the letterhead — logo (preferred) or band name — centred in `rect`.
// Used for the on-page header; the date is drawn separately as a page footer.
void drawLetterhead(QPainter &p, const QRectF &rect, const DocumentInfo &info);

#endif // DOCUMENTINFO_H