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

#ifndef UNDOCOMMANDS_H
#define UNDOCOMMANDS_H

#include "documentinfo.h"
#include "pageconfig.h"
#include "ports.h"
#include "stagescene.h"  // StageScene, DeviceTransform

#include <QList>
#include <QPointF>
#include <QString>
#include <QUndoCommand>
#include <QVector>

class DeviceItem;

// Add a single device. The command owns the item while it's not in the scene,
// so undo/redo move the same object in and out (no re-creation, stable pointer).
class AddDeviceCommand : public QUndoCommand
{
public:
    AddDeviceCommand(StageScene *scene, const QString &typeId, const QPointF &pos);
    ~AddDeviceCommand() override;
    void redo() override;
    void undo() override;

private:
    StageScene *m_scene;
    DeviceItem *m_item = nullptr;
    bool m_inScene = false;
};

// Delete one or more devices. The command keeps the removed items alive so undo
// can re-insert the very same objects.
class RemoveDevicesCommand : public QUndoCommand
{
public:
    RemoveDevicesCommand(StageScene *scene, const QList<DeviceItem *> &items);
    ~RemoveDevicesCommand() override;
    void redo() override;
    void undo() override;

private:
    StageScene *m_scene;
    QList<DeviceItem *> m_items;
    bool m_inScene = true;
};

// Move and/or rotate a set of devices (covers drag-move, drag-rotate, and the
// 15° rotate actions). The new state is already applied for a finished drag, so
// the first redo() is a harmless re-apply.
class TransformDevicesCommand : public QUndoCommand
{
public:
    TransformDevicesCommand(const QVector<DeviceTransform> &changes, const QString &text);
    void redo() override;
    void undo() override;

private:
    QVector<DeviceTransform> m_changes;
};

// Replace a device's label and ports (from the Properties panel).
class EditDeviceCommand : public QUndoCommand
{
public:
    EditDeviceCommand(StageScene *scene, DeviceItem *item, const QString &oldLabel,
                      const QList<Port> &oldPorts, const QString &newLabel,
                      const QList<Port> &newPorts);
    void redo() override;
    void undo() override;

private:
    void apply(const QString &label, const QList<Port> &ports);

    StageScene *m_scene;
    DeviceItem *m_item;
    QString m_oldLabel;
    QString m_newLabel;
    QList<Port> m_oldPorts;
    QList<Port> m_newPorts;
};

// Change the document's title-block metadata.
class EditDocumentInfoCommand : public QUndoCommand
{
public:
    EditDocumentInfoCommand(StageScene *scene, const DocumentInfo &oldInfo,
                            const DocumentInfo &newInfo);
    void redo() override;
    void undo() override;

private:
    StageScene *m_scene;
    DocumentInfo m_oldInfo;
    DocumentInfo m_newInfo;
};

// Change the page size/orientation.
class ChangePageConfigCommand : public QUndoCommand
{
public:
    ChangePageConfigCommand(StageScene *scene, const PageConfig &oldConfig,
                            const PageConfig &newConfig);
    void redo() override;
    void undo() override;

private:
    StageScene *m_scene;
    PageConfig m_oldConfig;
    PageConfig m_newConfig;
};

#endif // UNDOCOMMANDS_H