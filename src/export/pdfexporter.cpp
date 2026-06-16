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

#include "pdfexporter.h"

#include "documentinfo.h"
#include "inputlist.h"
#include "pageconfig.h"
#include "stagescene.h"

#include <QFont>
#include <QFontMetricsF>
#include <QGraphicsItem>
#include <QList>
#include <QObject>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QSignalBlocker>

namespace {

// Render the scene's page (header, plot, on-page legend, footer) into `area`
// without disturbing the live selection.
void renderScenePage(QPainter &p, StageScene *scene, const QRectF &area)
{
    const QList<QGraphicsItem *> previouslySelected = scene->selectedItems();
    const QSignalBlocker blocker(scene);
    scene->clearSelection();
    scene->render(&p, area, scene->pageRect(), Qt::KeepAspectRatio);
    for (QGraphicsItem *item : previouslySelected)
        item->setSelected(true);
}

} // namespace

namespace PdfExporter {

bool exportPlot(const QString &path, StageScene *scene, const QString &title, QString *error)
{
    if (!scene) {
        if (error)
            *error = QStringLiteral("No scene to export.");
        return false;
    }

    // Match the PDF page to the plot's own page setup so the canvas and export
    // agree (no letterboxing when the scene is rendered into the page).
    const PageConfig page = scene->pageConfig();

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(page.sizeId));
    writer.setPageOrientation(page.orientation);
    writer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    writer.setResolution(300);
    writer.setTitle(title);

    QPainter p;
    if (!p.begin(&writer)) {
        if (error)
            *error = QStringLiteral("Could not open %1 for writing.").arg(path);
        return false;
    }
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF area(0, 0, writer.width(), writer.height());

    const DocumentInfo info = scene->documentInfo();
    const QList<InputListEntry> entries = scene->inputListEntries();
    const int columns = inputlist::columnsFor(page.orientation);
    const bool overflow = scene->inputListShownCount() < entries.size();

    if (overflow && info.allowListOverflow) {
        // Big-rig split: the diagram gets its own page, the input-list legend
        // continues on following page(s).
        scene->setInputListVisible(false);
        renderScenePage(p, scene, area);
        scene->setInputListVisible(true);

        const double headerH = area.height() * 0.09;
        const double titleH = headerH * 1.25;
        QFont bodyFont = p.font();
        bodyFont.setPointSize(9);
        const double rowH = QFontMetricsF(bodyFont, p.device()).height() * 1.6;

        int idx = 0;
        bool firstLegendPage = true;
        while (idx < entries.size()) {
            writer.newPage();
            drawTitleBlock(p, QRectF(area.left(), area.top(), area.width(), headerH),
                           info, firstLegendPage ? QObject::tr("Input List")
                                                 : QObject::tr("Input List (cont.)"));
            const QRectF listArea(area.left(), area.top() + titleH, area.width(),
                                  area.bottom() - (area.top() + titleH));
            idx = inputlist::draw(&p, listArea, entries, columns, rowH,
                                  info.showPhantomInList, idx);
            firstLegendPage = false;
        }
    } else {
        // Everything (or as much as fits) on a single page — matches the canvas.
        // When it overflows but overflow isn't allowed, the scene draws a
        // "+N more" note rather than spilling onto another page.
        renderScenePage(p, scene, area);
    }

    p.end();
    return true;
}

} // namespace PdfExporter