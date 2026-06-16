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

#include "documentinfo.h"
#include "inputlist.h"
#include "pageconfig.h"

#include <QGraphicsScene>
#include <QList>
#include <QRectF>
#include <QString>

class DeviceCatalog;
class DeviceItem;
class QJsonObject;

// The stage page. Holds placed DeviceItems, accepts drops from the palette,
// draws the page, and serializes to/from JSON for save/load.
//
// MIME type for palette drops (carries a DeviceType id as UTF-8):
extern const char *const kDeviceMimeType;

// One row of the breakout / input list: a numbered console channel derived from
// a device's console-bound output port. Fields are pre-formatted for display.
struct Channel
{
    int number = 0;
    QString source;       // device label (+ " (L)" / " (R)" for stereo)
    QString connector;    // e.g. "XLR"
    QString signal;       // "Mono", "L", or "R"
    QString level;        // e.g. "Mic"
    bool balanced = true;
    bool phantom = false;
    QString providedBy;   // "Band" / "Venue" / "—"
    QString notes;
};

class StageScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit StageScene(const DeviceCatalog *catalog, QObject *parent = nullptr);

    // Create and add a device of the given type id at scenePos (centred there).
    // Returns the new item, or nullptr if the id is unknown.
    DeviceItem *addDevice(const QString &typeId, const QPointF &scenePos);

    QRectF pageRect() const { return m_pageRect; }

    // Page size/orientation for this plot. Setting it resizes the page (and the
    // scene rect) and keeps placed devices within the header clearance.
    void setPageConfig(const PageConfig &config);
    const PageConfig &pageConfig() const { return m_pageConfig; }

    void clearDevices();
    void removeDevices(const QList<QGraphicsItem *> &items);
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

protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;
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
};

#endif // STAGESCENE_H