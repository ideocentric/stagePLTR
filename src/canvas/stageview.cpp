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

#include "stageview.h"

#include <QPainter>
#include <QWheelEvent>

StageView::StageView(QWidget *parent)
    : QGraphicsView(parent)
{
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setAcceptDrops(true);
    setBackgroundBrush(QColor(0xd6, 0xd8, 0xdb));
}

void StageView::scaleBy(double factor)
{
    const double target = m_scale * factor;
    if (target < kMinScale || target > kMaxScale)
        return;
    m_scale = target;
    scale(factor, factor);
}

void StageView::zoomIn() { scaleBy(1.2); }
void StageView::zoomOut() { scaleBy(1.0 / 1.2); }

void StageView::resetZoom()
{
    resetTransform();
    m_scale = 1.0;
}

void StageView::wheelEvent(QWheelEvent *event)
{
    // Ctrl (Cmd on macOS) + wheel zooms; otherwise scroll normally.
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0)
            zoomIn();
        else if (event->angleDelta().y() < 0)
            zoomOut();
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}