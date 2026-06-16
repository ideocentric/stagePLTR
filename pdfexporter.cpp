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
#include "stagescene.h"

#include <QFont>
#include <QFontMetricsF>
#include <QGraphicsItem>
#include <QList>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QSignalBlocker>
#include <QStringList>
#include <QVector>

namespace {

struct ColumnSpec
{
    QString heading;
    double weight;        // fraction of the table width
    int alignment;        // Qt::Alignment flags
};

const QVector<ColumnSpec> &columns()
{
    static const QVector<ColumnSpec> cols = {
        {QStringLiteral("Ch"), 0.05, Qt::AlignCenter},
        {QStringLiteral("Source"), 0.20, Qt::AlignLeft | Qt::AlignVCenter},
        {QStringLiteral("Connector"), 0.11, Qt::AlignLeft | Qt::AlignVCenter},
        {QStringLiteral("Signal"), 0.07, Qt::AlignCenter},
        {QStringLiteral("Level"), 0.09, Qt::AlignLeft | Qt::AlignVCenter},
        {QStringLiteral("Bal."), 0.07, Qt::AlignCenter},
        {QStringLiteral("+48V"), 0.06, Qt::AlignCenter},
        {QStringLiteral("Provided By"), 0.12, Qt::AlignLeft | Qt::AlignVCenter},
        {QStringLiteral("Notes"), 0.23, Qt::AlignLeft | Qt::AlignVCenter},
    };
    return cols;
}

QStringList cellsFor(const Channel &ch)
{
    return {QString::number(ch.number),
            ch.source,
            ch.connector,
            ch.signal,
            ch.level,
            ch.balanced ? QStringLiteral("Bal") : QStringLiteral("Unbal"),
            ch.phantom ? QStringLiteral("+48V") : QString(),
            ch.providedBy,
            ch.notes};
}

// Draws the table header row at y and returns the y below it.
double drawTableHeader(QPainter &p, const QRectF &area, double y, double rowH,
                       const QVector<double> &x)
{
    QFont headFont = p.font();
    headFont.setPointSize(9);
    headFont.setBold(true);
    p.setFont(headFont);

    const QRectF headerRect(area.left(), y, area.width(), rowH);
    p.fillRect(headerRect, QColor(0xe8, 0xea, 0xed));

    const auto &cols = columns();
    const double pad = rowH * 0.25;
    p.setPen(QColor(0x33, 0x33, 0x33));
    for (int c = 0; c < cols.size(); ++c) {
        const QRectF cell(x[c], y, x[c + 1] - x[c], rowH);
        p.drawText(cell.adjusted(pad, 0, -pad, 0), cols[c].alignment, cols[c].heading);
    }
    p.setPen(QPen(QColor(0x88, 0x8a, 0x8d), 0));
    p.drawRect(headerRect);
    return y + rowH;
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

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::Letter));
    writer.setPageOrientation(QPageLayout::Landscape);
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
    const double headerH = area.height() * 0.09;

    // ---- Page 1: the stage plot (its title block is drawn by the scene) ------
    // Render the page region without selection highlights (signals blocked so
    // the UI's selection/properties state is untouched).
    const QList<QGraphicsItem *> previouslySelected = scene->selectedItems();
    {
        const QSignalBlocker blocker(scene);
        scene->clearSelection();
        scene->render(&p, area, scene->pageRect(), Qt::KeepAspectRatio);
        for (QGraphicsItem *item : previouslySelected)
            item->setSelected(true);
    }

    // ---- Page 2+: the input list --------------------------------------------
    writer.newPage();
    const double titleH = headerH * 1.25;
    drawTitleBlock(p, QRectF(area.left(), area.top(), area.width(), headerH),
                   scene->documentInfo(), QStringLiteral("Input List"));

    QFont bodyFont = p.font();
    bodyFont.setPointSize(9);
    const double rowH = QFontMetricsF(bodyFont, p.device()).height() * 1.6;

    // Precompute column x-edges.
    const auto &cols = columns();
    QVector<double> x(cols.size() + 1);
    x[0] = area.left();
    for (int c = 0; c < cols.size(); ++c)
        x[c + 1] = x[c] + cols[c].weight * area.width();

    double y = area.top() + titleH;
    y = drawTableHeader(p, area, y, rowH, x);

    const QList<Channel> chans = scene->channels();
    if (chans.isEmpty()) {
        p.setFont(bodyFont);
        p.setPen(QColor(0x66, 0x66, 0x66));
        p.drawText(QRectF(area.left(), y, area.width(), rowH),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("  No console channels yet."));
    }

    const double pad = rowH * 0.25;
    p.setFont(bodyFont);
    for (const Channel &ch : chans) {
        if (y + rowH > area.bottom()) {
            writer.newPage();
            drawTitleBlock(p, QRectF(area.left(), area.top(), area.width(), headerH),
                           scene->documentInfo(), QStringLiteral("Input List (cont.)"));
            y = area.top() + titleH;
            y = drawTableHeader(p, area, y, rowH, x);
            p.setFont(bodyFont);
        }

        const QStringList cells = cellsFor(ch);
        QFontMetricsF fm(bodyFont, p.device());
        for (int c = 0; c < cols.size(); ++c) {
            const QRectF cell(x[c], y, x[c + 1] - x[c], rowH);
            const QRectF textRect = cell.adjusted(pad, 0, -pad, 0);
            p.setPen(QColor(0x22, 0x22, 0x22));
            p.drawText(textRect, cols[c].alignment,
                       fm.elidedText(cells.at(c), Qt::ElideRight, textRect.width()));
            p.setPen(QPen(QColor(0xcc, 0xcc, 0xcc), 0));
            p.drawRect(cell);
        }
        y += rowH;
    }

    p.end();
    return true;
}

} // namespace PdfExporter