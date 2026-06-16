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

namespace {
constexpr qreal kLabelGap = 3.0;
constexpr qreal kLabelHeight = 26.0;     // room for up to two wrapped lines
constexpr qreal kLabelMinWidth = 100.0;  // labels may be wider than the icon
constexpr qreal kBadgeMargin = 4.0;
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
    // Keep the item below the title-block letterhead so the header never
    // covers a device.
    if (change == ItemPositionChange) {
        if (auto *stage = qobject_cast<StageScene *>(scene())) {
            QPointF pos = value.toPointF();
            const qreal minY = stage->contentTop() - boundingRect().top();
            if (pos.y() < minY)
                pos.setY(minY);
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

QRectF DeviceItem::boundingRect() const
{
    const qreal w = m_iconSize.width();
    const qreal h = m_iconSize.height();
    // Icon centred on origin; label sits below it in an area that may be wider
    // than the icon (so names aren't clipped). Extra top margin leaves room for
    // the channel badge that overhangs the icon corner.
    const qreal labelW = qMax(w, kLabelMinWidth);
    return QRectF(-labelW / 2.0, -h / 2.0 - kBadgeMargin,
                  labelW, h + kBadgeMargin + kLabelGap + kLabelHeight);
}

void DeviceItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                       QWidget *widget)
{
    Q_UNUSED(widget);

    const qreal w = m_iconSize.width();
    const qreal h = m_iconSize.height();
    const QRectF iconRect(-w / 2.0, -h / 2.0, w, h);

    if (m_renderer && m_renderer->isValid())
        m_renderer->render(painter, iconRect);

    // Label — centred below the icon in a fixed-width area, wrapping to two lines.
    if (!m_label.isEmpty()) {
        QFont font = painter->font();
        font.setPixelSize(11);  // pixel size = scene units, so it scales with the
        painter->setFont(font); // view/PDF render transform (not the device DPI)
        painter->setPen(QColor(0x22, 0x22, 0x22));
        const qreal labelW = qMax(w, kLabelMinWidth);
        const QRectF labelRect(-labelW / 2.0, h / 2.0 + kLabelGap, labelW, kLabelHeight);
        painter->drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
                          m_label);
    }

    // Channel-number badge at the icon's top-right corner.
    if (!m_channelBadge.isEmpty()) {
        QFont badgeFont = painter->font();
        badgeFont.setPixelSize(10);  // pixel size scales with the render transform
        badgeFont.setBold(true);
        painter->setFont(badgeFont);

        const qreal textW = QFontMetricsF(badgeFont).horizontalAdvance(m_channelBadge);
        const qreal r = 8.0;
        const qreal badgeW = qMax(2.0 * r, textW + 8.0);
        const QRectF badge(w / 2.0 - badgeW + 2.0, -h / 2.0 - 2.0, badgeW, 2.0 * r);

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0x1e, 0x73, 0xd6));
        painter->drawRoundedRect(badge, r, r);
        painter->setPen(Qt::white);
        painter->drawText(badge, Qt::AlignCenter, m_channelBadge);
    }

    // Selection highlight.
    if (option->state & QStyle::State_Selected) {
        QPen pen(QColor(0x1e, 0x73, 0xd6));
        pen.setStyle(Qt::DashLine);
        pen.setWidthF(1.0);
        pen.setCosmetic(true);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect().adjusted(0.5, 0.5, -0.5, -0.5));
    }
}