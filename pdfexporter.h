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

#ifndef PDFEXPORTER_H
#define PDFEXPORTER_H

#include <QString>

class StageScene;

// Exports a stage plot and its input list to a vector PDF (US Letter,
// landscape). Page 1 is the plot; the input-list table follows, paginating as
// needed. Rendering is pure Qt (QPdfWriter/QPainter) — no Chromium.
namespace PdfExporter {

// The document title (band name / logo + date) comes from the scene's
// DocumentInfo. `title` is used only for the PDF's internal metadata title.
bool exportPlot(const QString &path, StageScene *scene, const QString &title,
                QString *error = nullptr);

} // namespace PdfExporter

#endif // PDFEXPORTER_H
