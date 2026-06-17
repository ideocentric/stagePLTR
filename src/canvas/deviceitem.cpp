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

#include "deviceitem.h"

#include "stagescene.h"

#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QSvgRenderer>
#include <QVariant>
#include <QtMath>

#include <cmath>

namespace {
constexpr qreal kLabelGap = 3.0;
constexpr qreal kLabelHeight = 26.0;     // room for up to two wrapped lines
constexpr qreal kLabelMinWidth = 100.0;  // labels may be wider than the icon
constexpr qreal kBadgeReserve = 18.0;    // padding for the upright badge swing

// Axis-aligned half-extents of a w×h box rotated by `degrees` (about its centre).
QSizeF rotatedHalfExtents(qreal w, qreal h, qreal degrees)
{
    const qreal rad = qDegreesToRadians(degrees);
    const qreal c = std::abs(std::cos(rad));
    const qreal s = std::abs(std::sin(rad));
    return QSizeF((w / 2.0) * c + (h / 2.0) * s, (w / 2.0) * s + (h / 2.0) * c);
}
}

DeviceItem::DeviceItem(const DeviceType &type, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_typeId(type.id)
    , m_label(type.name)
    , m_iconSize(type.defaultSize)
    , m_ports(type.ports)
{
    m_renderer = new QSvgRenderer(type.iconResource, this);

    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
    setTransformOriginPoint(0.0, 0.0);  // origin is the icon centre; rotation pivots here
    setCursor(Qt::OpenHandCursor);
    setToolTip(type.name);
}

void DeviceItem::setLabel(const QString &label)
{
    if (m_label == label)
        return;
    prepareGeometryChange();
    m_label = label;
    update();
}

void DeviceItem::setPorts(const QList<Port> &ports)
{
    m_ports = ports;
    update();
}

QVariant DeviceItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    // Keep the item within the stage area: below the letterhead (top) and above
    // the on-page input-list legend (bottom). Clamp by the icon rect, not the
    // (much larger) bounding rect, so the reserved label/badge room doesn't
    // over-constrain placement.
    if (change == ItemPositionChange) {
        if (auto *stage = qobject_cast<StageScene *>(scene())) {
            QPointF pos = value.toPointF();
            const QRectF icon = iconRect();
            const qreal minY = stage->contentTop() - icon.top();
            const qreal maxY = stage->contentBottom() - icon.bottom();
            pos.setY(qBound(minY, pos.y(), qMax(minY, maxY)));
            return pos;
        }
    }
    return QGraphicsObject::itemChange(change, value);
}

void DeviceItem::setChannelBadge(const QString &badge)
{
    if (m_channelBadge == badge)
        return;
    m_channelBadge = badge;
    update();
}

QRectF DeviceItem::iconRect() const
{
    const qreal w = m_iconSize.width();
    const qreal h = m_iconSize.height();
    return QRectF(-w / 2.0, -h / 2.0, w, h);
}

QRectF DeviceItem::boundingRect() const
{
    // The icon rotates with the item, but the label and badge are drawn upright
    // (counter-rotated) and re-anchored to the icon's rotated footprint. So the
    // bounding rect must contain those annotations for *any* rotation: use a
    // rotation-invariant square large enough to hold the icon's diagonal plus
    // the label below and the badge at the corner.
    const qreal w = m_iconSize.width();
    const qreal h = m_iconSize.height();
    const qreal labelW = qMax(w, kLabelMinWidth);
    const qreal diagHalf = 0.5 * std::hypot(w, h);
    const qreal labelReach = diagHalf + kLabelGap + kLabelHeight;
    const qreal reach =
        std::hypot(labelW / 2.0, labelReach) + kBadgeReserve;
    return QRectF(-reach, -reach, 2.0 * reach, 2.0 * reach);
}

QPainterPath DeviceItem::shape() const
{
    // Hit-test (and tight selection) against the icon only, not the large
    // bounding rect — so clicks land on the symbol, not its reserved halo.
    QPainterPath path;
    path.addRect(iconRect());
    return path;
}

void DeviceItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                       QWidget *widget)
{
    Q_UNUSED(widget);

    const qreal w = m_iconSize.width();
    const qreal h = m_iconSize.height();

    // The icon rotates with the item.
    if (m_renderer && m_renderer->isValid())
        m_renderer->render(painter, iconRect());

    // Selection highlight: a tight dashed box around the (rotated) icon.
    if (option->state & QStyle::State_Selected) {
        QPen pen(QColor(0x1e, 0x73, 0xd6));
        pen.setStyle(Qt::DashLine);
        pen.setWidthF(1.0);
        pen.setCosmetic(true);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(iconRect().adjusted(-1.0, -1.0, 1.0, 1.0));
    }

    // Label and badge stay upright regardless of the object's rotation. Counter-
    // rotate into a scene-aligned frame and re-anchor to the icon's rotated
    // footprint (axis-aligned half-extents), so the label sits below the symbol
    // and the badge at its top-right corner whatever the angle.
    const QSizeF half = rotatedHalfExtents(w, h, rotation());

    painter->save();
    painter->rotate(-rotation());

    if (!m_label.isEmpty()) {
        QFont font = painter->font();
        font.setPixelSize(11);  // scene units, so it scales with the view/PDF transform
        painter->setFont(font);
        painter->setPen(QColor(0x22, 0x22, 0x22));
        const qreal labelW = qMax(w, kLabelMinWidth);
        const QRectF labelRect(-labelW / 2.0, half.height() + kLabelGap, labelW,
                               kLabelHeight);
        painter->drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
                          m_label);
    }

    if (!m_channelBadge.isEmpty()) {
        QFont badgeFont = painter->font();
        badgeFont.setPixelSize(10);
        badgeFont.setBold(true);
        painter->setFont(badgeFont);

        const qreal textW = QFontMetricsF(badgeFont).horizontalAdvance(m_channelBadge);
        const qreal r = 8.0;
        const qreal badgeW = qMax(2.0 * r, textW + 8.0);
        const QRectF badge(half.width() - badgeW + 2.0, -half.height() - 2.0,
                           badgeW, 2.0 * r);

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0x1e, 0x73, 0xd6));
        painter->drawRoundedRect(badge, r, r);
        painter->setPen(Qt::white);
        painter->drawText(badge, Qt::AlignCenter, m_channelBadge);
    }

    painter->restore();
}