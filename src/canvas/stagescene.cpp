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

#include "stagescene.h"

#include "devicecatalog.h"
#include "deviceitem.h"
#include "documentinfo.h"
#include "ports.h"

#include <QFont>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QJsonArray>
#include <QJsonObject>
#include <QMimeData>
#include <QPainter>
#include <QTransform>

const char *const kDeviceMimeType = "application/x-stageplt-device";

namespace {
// Page units are pixels at 96 dpi; the page rect comes from the PageConfig.
constexpr qreal kMargin = 48.0;

// Letterhead geometry (top of the page) and footer (bottom of the page).
constexpr qreal kHeaderHeight = 60.0;
constexpr qreal kHeaderPad = 16.0;
// Vertical space devices must stay below when a letterhead is shown.
constexpr qreal kHeaderReserved = kHeaderHeight + 2 * kHeaderPad + 4.0;
constexpr qreal kFooterHeight = 22.0;  // small centred date strip at the bottom

// On-page condensed input-list legend (below the plot, above the footer).
constexpr qreal kListRowH = 16.0;      // one condensed line, in scene pixels
constexpr qreal kListTitleH = 20.0;    // "Input List" caption + divider
constexpr qreal kListPad = 8.0;        // gap between stage area and the legend
constexpr qreal kListMaxFrac = 0.42;   // legend never takes more than this much
}

StageScene::StageScene(const DeviceCatalog *catalog, QObject *parent)
    : QGraphicsScene(parent)
    , m_catalog(catalog)
{
    setPageConfig(m_pageConfig);  // establishes m_pageRect and the scene rect
}

void StageScene::setPageConfig(const PageConfig &config)
{
    m_pageConfig = config;
    const QSizeF px = config.pixelSize();
    m_pageRect = QRectF(0.0, 0.0, px.width(), px.height());
    setSceneRect(m_pageRect.adjusted(-kMargin, -kMargin, kMargin, kMargin));
    enforceContentBounds();  // keep devices clear of the (possibly moved) header/legend
    update();
}

int StageScene::inputListColumns() const
{
    return inputlist::columnsFor(m_pageConfig.orientation);
}

qreal StageScene::inputListBandTop() const
{
    const qreal footerTop = m_pageRect.bottom() - kFooterHeight;
    if (m_listEntries.isEmpty() || !m_inputListVisible)
        return footerTop;
    const int cols = inputListColumns();
    const int rows = (m_listEntries.size() + cols - 1) / cols;
    qreal height = kListTitleH + rows * kListRowH + kListPad;
    height = qMin(height, m_pageRect.height() * kListMaxFrac);
    return footerTop - height;
}

QRectF StageScene::inputListRect() const
{
    const qreal top = inputListBandTop() + kListTitleH;
    const qreal bottom = m_pageRect.bottom() - kFooterHeight;
    return QRectF(m_pageRect.left() + kHeaderPad, top,
                  m_pageRect.width() - 2 * kHeaderPad, qMax(0.0, bottom - top));
}

int StageScene::inputListShownCount() const
{
    // Null painter => measure only: returns the count that fits in the band.
    return inputlist::draw(nullptr, inputListRect(), m_listEntries, inputListColumns(),
                           kListRowH, m_documentInfo.showPhantomInList, 0);
}

void StageScene::setInputListVisible(bool visible)
{
    if (m_inputListVisible == visible)
        return;
    m_inputListVisible = visible;
    update();
}

DeviceItem *StageScene::makeDevice(const QString &typeId)
{
    if (!m_catalog)
        return nullptr;
    const DeviceType *type = m_catalog->find(typeId);
    if (!type)
        return nullptr;

    auto *item = new DeviceItem(*type);
    // Any later geometry change on the item is a content change. Wired once at
    // creation; the connections persist across remove/re-insert (undo/redo).
    connect(item, &QGraphicsObject::xChanged, this, &StageScene::plotChanged);
    connect(item, &QGraphicsObject::yChanged, this, &StageScene::plotChanged);
    connect(item, &QGraphicsObject::rotationChanged, this, &StageScene::plotChanged);
    return item;
}

void StageScene::insertDevice(DeviceItem *item)
{
    if (!item)
        return;
    addItem(item);
    emit plotChanged();
    renumberChannels();  // also re-clamps the item into the stage area
}

void StageScene::removeDeviceItem(DeviceItem *item)
{
    if (!item)
        return;
    removeItem(item);  // no delete — ownership returns to the caller (a command)
    emit plotChanged();
    renumberChannels();
}

DeviceItem *StageScene::addDevice(const QString &typeId, const QPointF &scenePos)
{
    DeviceItem *item = makeDevice(typeId);
    if (!item)
        return nullptr;
    item->setPos(scenePos);
    insertDevice(item);
    return item;
}

void StageScene::clearDevices()
{
    const QList<QGraphicsItem *> existing = items();
    bool removedAny = false;
    for (QGraphicsItem *item : existing) {
        if (item->type() == DeviceItem::Type) {
            removeItem(item);
            delete item;
            removedAny = true;
        }
    }
    if (removedAny) {
        emit plotChanged();
        renumberChannels();
    }
}

QJsonObject StageScene::toJson() const
{
    QJsonArray devices;
    // Iterate in stable (insertion-ish) order: items() is z-ordered, which is
    // fine for round-tripping. Numbering order will matter once ports land.
    const QList<QGraphicsItem *> all = items(Qt::AscendingOrder);
    for (QGraphicsItem *item : all) {
        if (item->type() != DeviceItem::Type)
            continue;
        const auto *device = static_cast<DeviceItem *>(item);
        QJsonObject obj;
        obj[QStringLiteral("type")] = device->typeId();
        obj[QStringLiteral("x")] = device->pos().x();
        obj[QStringLiteral("y")] = device->pos().y();
        obj[QStringLiteral("rotation")] = device->rotation();
        obj[QStringLiteral("label")] = device->label();

        QJsonArray portArray;
        for (const Port &port : device->ports())
            portArray.append(port.toJson());
        obj[QStringLiteral("ports")] = portArray;

        devices.append(obj);
    }

    QJsonObject root;
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("page")] = m_pageConfig.toJson();
    root[QStringLiteral("devices")] = devices;
    return root;
}

bool StageScene::fromJson(const QJsonObject &obj, QString *error)
{
    clearDevices();

    // Restore the page setup before placing devices so clamping uses the right
    // page. Files without a "page" key keep the current (global-default) config.
    if (obj.contains(QStringLiteral("page")))
        setPageConfig(PageConfig::fromJson(obj.value(QStringLiteral("page")).toObject()));

    const QJsonArray devices = obj.value(QStringLiteral("devices")).toArray();
    for (const QJsonValue &value : devices) {
        const QJsonObject d = value.toObject();
        const QString typeId = d.value(QStringLiteral("type")).toString();
        DeviceItem *item =
            addDevice(typeId, QPointF(d.value(QStringLiteral("x")).toDouble(),
                                      d.value(QStringLiteral("y")).toDouble()));
        if (!item) {
            // Unknown device id: skip but report so the user knows something was dropped.
            if (error)
                *error = QStringLiteral("Unknown device type '%1' skipped").arg(typeId);
            continue;
        }
        item->setRotation(d.value(QStringLiteral("rotation")).toDouble());
        const QString label = d.value(QStringLiteral("label")).toString();
        if (!label.isEmpty())
            item->setLabel(label);

        // Restore saved ports if present; otherwise keep the catalog defaults.
        if (d.contains(QStringLiteral("ports"))) {
            QList<Port> restored;
            const QJsonArray portArray = d.value(QStringLiteral("ports")).toArray();
            for (const QJsonValue &portValue : portArray)
                restored.append(Port::fromJson(portValue.toObject()));
            item->setPorts(restored);
        }
    }
    renumberChannels();
    return true;
}

void StageScene::renumberChannels()
{
    m_channels.clear();
    m_listEntries.clear();
    int next = 1;

    const QList<QGraphicsItem *> all = items(Qt::AscendingOrder);
    for (QGraphicsItem *gi : all) {
        if (gi->type() != DeviceItem::Type)
            continue;
        auto *device = static_cast<DeviceItem *>(gi);

        QList<int> deviceNumbers;
        for (const Port &port : device->ports()) {
            if (!port.isConsoleChannel())
                continue;
            const bool stereo = port.signal == SignalConfig::Stereo;
            const int count = port.channelCount();
            const int portFirst = next;
            for (int i = 0; i < count; ++i) {
                Channel ch;
                ch.number = next;
                ch.source = device->label();
                if (stereo) {
                    ch.signal = (i == 0) ? QStringLiteral("L") : QStringLiteral("R");
                    ch.source += (i == 0) ? QStringLiteral(" (L)") : QStringLiteral(" (R)");
                } else {
                    ch.signal = QStringLiteral("Mono");
                }
                ch.connector = ports::display(port.connector);
                ch.level = ports::display(port.level);
                ch.balanced = port.balanced;
                ch.phantom = port.phantom;
                ch.providedBy = ports::display(port.providedBy);
                ch.notes = port.notes;
                m_channels.append(ch);
                deviceNumbers.append(next);
                ++next;
            }

            // One condensed legend entry per console port; stereo is a range.
            InputListEntry entry;
            entry.numbers = (count > 1)
                ? QStringLiteral("%1–%2").arg(portFirst).arg(next - 1)
                : QString::number(portFirst);
            entry.signalWord = stereo ? tr("Stereo") : tr("Mono");
            entry.connector = ports::display(port.connector);
            entry.level = ports::display(port.level);
            entry.phantom = port.phantom;
            m_listEntries.append(entry);
        }

        // Badge: compact form — "" / "3" / "3–4" (contiguous) / "3,5".
        QString badge;
        if (deviceNumbers.size() == 1) {
            badge = QString::number(deviceNumbers.first());
        } else if (deviceNumbers.size() > 1) {
            const bool contiguous =
                deviceNumbers.last() - deviceNumbers.first() == deviceNumbers.size() - 1;
            if (contiguous)
                badge = QStringLiteral("%1–%2").arg(deviceNumbers.first()).arg(deviceNumbers.last());
            else {
                QStringList parts;
                for (int n : deviceNumbers)
                    parts << QString::number(n);
                badge = parts.join(QLatin1Char(','));
            }
        }
        device->setChannelBadge(badge);
    }

    // The legend size changed, so the stage area's bottom moved.
    enforceContentBounds();
    emit channelsChanged();
}

void StageScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat(QString::fromLatin1(kDeviceMimeType)))
        event->acceptProposedAction();
    else
        QGraphicsScene::dragEnterEvent(event);
}

void StageScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat(QString::fromLatin1(kDeviceMimeType)))
        event->acceptProposedAction();
    else
        QGraphicsScene::dragMoveEvent(event);
}

void StageScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    const QMimeData *mime = event->mimeData();
    if (!mime->hasFormat(QString::fromLatin1(kDeviceMimeType))) {
        QGraphicsScene::dropEvent(event);
        return;
    }
    const QString typeId =
        QString::fromUtf8(mime->data(QString::fromLatin1(kDeviceMimeType)));
    // Let the window turn this into an undoable add rather than mutating here.
    emit dropRequested(typeId, event->scenePos());
    event->acceptProposedAction();
}

void StageScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mousePressEvent(event);  // settles selection / starts the drag

    // Snapshot the selected devices so a completed drag becomes one undo step.
    m_dragStart.clear();
    if (event->button() == Qt::LeftButton) {
        const QList<QGraphicsItem *> selected = selectedItems();
        for (QGraphicsItem *gi : selected) {
            if (gi->type() != DeviceItem::Type)
                continue;
            auto *device = static_cast<DeviceItem *>(gi);
            m_dragStart.insert(device, {device->pos(), device->rotation()});
        }
    }
}

void StageScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseReleaseEvent(event);

    if (m_dragStart.isEmpty())
        return;

    QVector<DeviceTransform> changes;
    for (auto it = m_dragStart.cbegin(); it != m_dragStart.cend(); ++it) {
        DeviceItem *device = it.key();
        const QPointF oldPos = it.value().first;
        const qreal oldRot = it.value().second;
        if (device->pos() != oldPos || device->rotation() != oldRot)
            changes.append({device, oldPos, oldRot, device->pos(), device->rotation()});
    }
    m_dragStart.clear();
    if (!changes.isEmpty())
        emit devicesTransformed(changes);
}

void StageScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, QColor(0xd6, 0xd8, 0xdb));   // canvas surround

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->fillRect(m_pageRect, Qt::white);            // the page

    QPen border(QColor(0x9a, 0x9d, 0xa1));
    border.setCosmetic(true);
    painter->setPen(border);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(m_pageRect);
}

void StageScene::setDocumentInfo(const DocumentInfo &info)
{
    m_documentInfo = info;
    enforceContentBounds();
    update();
}

bool StageScene::headerShown() const
{
    return m_documentInfo.hasContent();  // a logo or band name to display
}

QRectF StageScene::headerRect() const
{
    return QRectF(m_pageRect.left() + kHeaderPad, m_pageRect.top() + kHeaderPad,
                  m_pageRect.width() - 2 * kHeaderPad, kHeaderHeight);
}

qreal StageScene::contentTop() const
{
    // Reserve the letterhead band only when there is actually a logo/name to
    // show; the empty-state hint is faint and doesn't push devices down.
    return headerShown() ? m_pageRect.top() + kHeaderReserved : m_pageRect.top();
}

qreal StageScene::contentBottom() const
{
    // Stage area ends above the on-page legend (or the footer when there's none).
    return inputListBandTop() - kListPad;
}

void StageScene::enforceContentBounds()
{
    const qreal top = contentTop();
    const qreal bottom = contentBottom();
    const QList<QGraphicsItem *> all = items();
    for (QGraphicsItem *gi : all) {
        if (gi->type() != DeviceItem::Type)
            continue;
        const QRectF icon = static_cast<DeviceItem *>(gi)->iconRect();
        const qreal minY = top - icon.top();
        const qreal maxY = bottom - icon.bottom();
        // itemChange re-clamps too; this nudges existing devices into range.
        const qreal y = qBound(minY, gi->pos().y(), qMax(minY, maxY));
        if (gi->pos().y() != y)
            gi->setPos(gi->pos().x(), y);
    }
}

void StageScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawForeground(painter, rect);
    drawHeader(*painter);
    drawInputList(*painter);
    drawFooter(*painter);
    Q_UNUSED(rect);
}

void StageScene::drawInputList(QPainter &painter) const
{
    if (m_listEntries.isEmpty() || !m_inputListVisible)
        return;

    const qreal bandTop = inputListBandTop();
    const QRectF area = inputListRect();
    painter.save();

    // Divider + caption above the columns.
    QPen rule(QColor(0xb5, 0xb8, 0xbb));
    rule.setCosmetic(true);
    painter.setPen(rule);
    painter.drawLine(QPointF(m_pageRect.left() + kHeaderPad, bandTop),
                     QPointF(m_pageRect.right() - kHeaderPad, bandTop));
    QFont caption = painter.font();
    caption.setPixelSize(11);
    caption.setBold(true);
    painter.setFont(caption);
    painter.setPen(QColor(0x44, 0x47, 0x4a));
    painter.drawText(QRectF(m_pageRect.left() + kHeaderPad, bandTop + 2.0,
                            m_pageRect.width() - 2 * kHeaderPad, kListTitleH - 2.0),
                     Qt::AlignLeft | Qt::AlignVCenter, tr("Input List"));

    const int shown = inputlist::draw(&painter, area, m_listEntries, inputListColumns(),
                                      kListRowH, m_documentInfo.showPhantomInList, 0);
    if (shown < m_listEntries.size()) {
        // Tell the user the rest continues on the exported legend page(s).
        QFont note = painter.font();
        note.setPixelSize(9);
        note.setBold(false);
        note.setItalic(true);
        painter.setFont(note);
        painter.setPen(QColor(0x90, 0x93, 0x96));
        painter.drawText(QRectF(m_pageRect.left() + kHeaderPad,
                                m_pageRect.bottom() - kFooterHeight - 12.0,
                                m_pageRect.width() - 2 * kHeaderPad, 12.0),
                         Qt::AlignRight | Qt::AlignVCenter,
                         tr("+%1 more (see additional pages)")
                             .arg(m_listEntries.size() - shown));
    }
    painter.restore();
}

void StageScene::drawHeader(QPainter &painter) const
{
    const QRectF band = headerRect();
    painter.save();
    if (headerShown()) {
        // Logo or band name, centred at the top of the page (part of the page,
        // not a separate filled strip).
        drawLetterhead(painter, band, m_documentInfo);
    } else {
        // Discoverability: faint hint that opens Document Info on double-click.
        QFont font = painter.font();
        font.setPixelSize(13);
        font.setItalic(true);
        painter.setFont(font);
        painter.setPen(QColor(0xb0, 0xb3, 0xb6));
        painter.drawText(band, Qt::AlignCenter,
                         QObject::tr("Double-click to add band name / logo"));
    }
    painter.restore();
}

void StageScene::drawFooter(QPainter &painter) const
{
    if (!m_documentInfo.date.isValid())
        return;
    painter.save();
    QFont font = painter.font();
    font.setPixelSize(9);  // very small, centred footer
    painter.setFont(font);
    painter.setPen(QColor(0x80, 0x80, 0x80));
    const QRectF footer(m_pageRect.left(), m_pageRect.bottom() - kFooterHeight,
                        m_pageRect.width(), kFooterHeight);
    painter.drawText(footer, Qt::AlignHCenter | Qt::AlignVCenter,
                     m_documentInfo.date.toString(QStringLiteral("MMMM d, yyyy")));
    painter.restore();
}

void StageScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    // Double-clicking the header band (and not a device) edits the band/logo.
    if (event->button() == Qt::LeftButton
        && !itemAt(event->scenePos(), QTransform())
        && headerRect().contains(event->scenePos())) {
        emit editDocumentInfoRequested();
        event->accept();
        return;
    }
    QGraphicsScene::mouseDoubleClickEvent(event);
}