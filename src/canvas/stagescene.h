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

#ifndef STAGESCENE_H
#define STAGESCENE_H

#include "channel.h"
#include "documentinfo.h"
#include "inputlist.h"
#include "pageconfig.h"

#include <QGraphicsScene>
#include <QHash>
#include <QList>
#include <QPair>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

class DeviceCatalog;
class DeviceItem;
class QJsonObject;

// The stage page. Holds placed DeviceItems, accepts drops from the palette,
// draws the page, and serializes to/from JSON for save/load.
//
// MIME type for palette drops (carries a DeviceType id as UTF-8):
extern const char *const kDeviceMimeType;

// A device's position/rotation before and after a user drag (or a rotate
// command). Emitted via devicesTransformed() so the window can record one undo
// step per completed gesture.
struct DeviceTransform
{
    DeviceItem *item = nullptr;
    QPointF oldPos;
    qreal oldRotation = 0.0;
    QPointF newPos;
    qreal newRotation = 0.0;
};

class StageScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit StageScene(const DeviceCatalog *catalog, QObject *parent = nullptr);

    // Create and add a device of the given type id at scenePos (centred there).
    // Returns the new item, or nullptr if the id is unknown.
    DeviceItem *addDevice(const QString &typeId, const QPointF &scenePos);

    // Lower-level primitives used by undo commands: create a device without
    // inserting it (caller owns it until inserted), and move an existing item
    // in/out of the scene without deleting it (so a command can keep it alive).
    DeviceItem *makeDevice(const QString &typeId);
    void insertDevice(DeviceItem *item);
    void removeDeviceItem(DeviceItem *item);

    QRectF pageRect() const { return m_pageRect; }

    // Page size/orientation for this plot. Setting it resizes the page (and the
    // scene rect) and keeps placed devices within the header clearance.
    void setPageConfig(const PageConfig &config);
    const PageConfig &pageConfig() const { return m_pageConfig; }

    void clearDevices();
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject &obj, QString *error = nullptr);

    // Recompute channel numbers across all console-bound output ports (in scene
    // stacking order), update each device's badge, and refresh the channel list.
    void renumberChannels();
    const QList<Channel> &channels() const { return m_channels; }

    // Condensed input-list entries (stereo grouped), for the on-page legend and
    // the PDF. Built alongside renumberChannels() so they match the badges.
    const QList<InputListEntry> &inputListEntries() const { return m_listEntries; }
    int inputListColumns() const;       // by orientation: 2 portrait, 4 landscape
    QRectF inputListRect() const;       // entry area of the on-page legend band
    int inputListShownCount() const;    // entries that fit in the on-page legend
    // Suppress the on-page legend (used when the PDF splits plot/legend pages).
    void setInputListVisible(bool visible);

    // Document metadata for the title block (drawn at the top of the page).
    void setDocumentInfo(const DocumentInfo &info);
    const DocumentInfo &documentInfo() const { return m_documentInfo; }

    // The usable stage area for devices, kept clear of the letterhead (top) and
    // the on-page input-list legend (bottom).
    qreal contentTop() const;
    qreal contentBottom() const;

signals:
    // Emitted on any user-visible content change (add / move / rotate / delete).
    void plotChanged();
    // Emitted when the numbered channel list changes (for the breakout table).
    void channelsChanged();
    // Emitted when the user double-clicks the page header (to edit band/logo).
    void editDocumentInfoRequested();
    // A palette item was dropped; the window records it as an undoable add.
    void dropRequested(const QString &typeId, const QPointF &scenePos);
    // One or more devices finished a move/rotate drag (for one undo step).
    void devicesTransformed(const QVector<DeviceTransform> &changes);

protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;

private:
    bool headerShown() const;          // a logo or band name is set
    QRectF headerRect() const;         // top-of-page letterhead band
    qreal inputListBandTop() const;    // top of the on-page legend (incl. title)
    void drawHeader(QPainter &painter) const;
    void drawFooter(QPainter &painter) const;
    void drawInputList(QPainter &painter) const;
    void enforceContentBounds();       // clamp devices to [contentTop, contentBottom]

    const DeviceCatalog *m_catalog;
    PageConfig m_pageConfig;
    QRectF m_pageRect;
    QList<Channel> m_channels;
    QList<InputListEntry> m_listEntries;
    DocumentInfo m_documentInfo;
    bool m_inputListVisible = true;

    // Transforms of the selected devices captured at mouse-press, to diff on
    // release into a single move/rotate undo step.
    QHash<DeviceItem *, QPair<QPointF, qreal>> m_dragStart;
};

#endif // STAGESCENE_H