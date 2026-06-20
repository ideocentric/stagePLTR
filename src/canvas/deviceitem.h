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

#ifndef DEVICEITEM_H
#define DEVICEITEM_H

#include "devicetype.h"
#include "ports.h"

#include <QGraphicsObject>
#include <QList>
#include <QPainterPath>
#include <QPixmap>
#include <QSizeF>
#include <QString>

class QSvgRenderer;

// A device placed on the stage: renders its SVG symbol with a label below.
// Movable, selectable, and rotatable. The origin (0,0) is the icon centre, so
// rotation pivots about the symbol.
class DeviceItem : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit DeviceItem(const DeviceType &type, QGraphicsItem *parent = nullptr);

    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    QString typeId() const { return m_typeId; }

    QString label() const { return m_label; }
    void setLabel(const QString &label);

    const QList<Port> &ports() const { return m_ports; }
    void setPorts(const QList<Port> &ports);

    // Compact channel-number string drawn as a badge (e.g. "3" or "3–4"),
    // assigned by the scene during renumbering. Empty = no badge.
    void setChannelBadge(const QString &badge);

    // User offset of the label from its default spot below the icon. Stored
    // relative to the device (so it moves and rotates-anchors with it); empty =
    // the default position. The label text stays upright regardless.
    QPointF labelOffset() const { return m_labelOffset; }
    void setLabelOffset(const QPointF &offset);

    // The label's rectangle in scene coordinates, and a hit-test against it
    // (used to drag the label and to double-click-reset it).
    QRectF labelSceneRect() const;
    bool labelContains(const QPointF &scenePoint) const;

    // The icon's own rectangle (centred on the origin), used for hit-testing and
    // for keeping the device within the stage area — independent of the larger
    // bounding rect that must also reserve room for the upright label/badge.
    QRectF iconRect() const;

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    // Scene position of the rotation handle (above the icon's footprint), and a
    // hit-test against it. Only meaningful while the item is selected.
    QPointF rotationHandleScenePos() const;
    // Scene-aligned vector from the device centre to the label centre, including
    // the user offset.
    QPointF labelCentreVector() const;

    QString m_typeId;
    QString m_label;
    QSizeF m_iconSize;
    QList<Port> m_ports;
    QString m_channelBadge;
    QPointF m_labelOffset;               // user displacement from the default spot
    QSvgRenderer *m_renderer = nullptr;  // owned (child QObject); SVG icons
    QPixmap m_iconPixmap;                // raster icons (when not SVG)
    bool m_rotating = false;             // dragging the rotation handle
    bool m_movingLabel = false;          // dragging the label
    QPointF m_labelGrabScene;            // cursor->label-centre delta while dragging
};

#endif // DEVICEITEM_H