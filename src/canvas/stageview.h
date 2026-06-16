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

#ifndef STAGEVIEW_H
#define STAGEVIEW_H

#include <QGraphicsView>

// Canvas viewport: antialiased rendering, rubber-band selection, and
// Ctrl/Cmd + mouse-wheel zoom. Accepts drops so the scene can place devices.
class StageView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit StageView(QWidget *parent = nullptr);

    void zoomIn();
    void zoomOut();
    void resetZoom();

protected:
    void wheelEvent(QWheelEvent *event) override;

private:
    void scaleBy(double factor);

    double m_scale = 1.0;
    static constexpr double kMinScale = 0.2;
    static constexpr double kMaxScale = 5.0;
};

#endif // STAGEVIEW_H