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

#include "undocommands.h"

#include "deviceitem.h"

#include <QObject>

// ---- AddDeviceCommand -------------------------------------------------------

AddDeviceCommand::AddDeviceCommand(StageScene *scene, const QString &typeId,
                                   const QPointF &pos)
    : m_scene(scene)
{
    m_item = scene->makeDevice(typeId);  // created but not yet in the scene
    if (m_item) {
        m_item->setPos(pos);
        setText(QObject::tr("Add %1").arg(m_item->label()));
    } else {
        setText(QObject::tr("Add Device"));
    }
}

AddDeviceCommand::~AddDeviceCommand()
{
    if (m_item && !m_inScene)
        delete m_item;  // we still own it (currently undone)
}

void AddDeviceCommand::redo()
{
    if (m_item) {
        m_scene->insertDevice(m_item);
        m_inScene = true;
    }
}

void AddDeviceCommand::undo()
{
    if (m_item) {
        m_scene->removeDeviceItem(m_item);
        m_inScene = false;
    }
}

// ---- RemoveDevicesCommand ---------------------------------------------------

RemoveDevicesCommand::RemoveDevicesCommand(StageScene *scene,
                                           const QList<DeviceItem *> &items)
    : m_scene(scene)
    , m_items(items)
{
    setText(items.size() == 1 ? QObject::tr("Delete Device")
                              : QObject::tr("Delete %1 Devices").arg(items.size()));
}

RemoveDevicesCommand::~RemoveDevicesCommand()
{
    if (!m_inScene)
        qDeleteAll(m_items);  // removed and still owned by us
}

void RemoveDevicesCommand::redo()
{
    for (DeviceItem *item : std::as_const(m_items))
        m_scene->removeDeviceItem(item);
    m_inScene = false;
}

void RemoveDevicesCommand::undo()
{
    for (DeviceItem *item : std::as_const(m_items))
        m_scene->insertDevice(item);
    m_inScene = true;
}

// ---- TransformDevicesCommand ------------------------------------------------

TransformDevicesCommand::TransformDevicesCommand(const QVector<DeviceTransform> &changes,
                                                 const QString &text)
    : m_changes(changes)
{
    setText(text);
}

void TransformDevicesCommand::redo()
{
    for (const DeviceTransform &c : std::as_const(m_changes)) {
        c.item->setPos(c.newPos);
        c.item->setRotation(c.newRotation);
    }
}

void TransformDevicesCommand::undo()
{
    for (const DeviceTransform &c : std::as_const(m_changes)) {
        c.item->setPos(c.oldPos);
        c.item->setRotation(c.oldRotation);
    }
}

// ---- EditDeviceCommand ------------------------------------------------------

EditDeviceCommand::EditDeviceCommand(StageScene *scene, DeviceItem *item,
                                     const QString &oldLabel, const QList<Port> &oldPorts,
                                     const QString &newLabel, const QList<Port> &newPorts)
    : m_scene(scene)
    , m_item(item)
    , m_oldLabel(oldLabel)
    , m_newLabel(newLabel)
    , m_oldPorts(oldPorts)
    , m_newPorts(newPorts)
{
    setText(QObject::tr("Edit %1").arg(newLabel.isEmpty() ? oldLabel : newLabel));
}

void EditDeviceCommand::apply(const QString &label, const QList<Port> &ports)
{
    m_item->setLabel(label);
    m_item->setPorts(ports);
    m_scene->renumberChannels();
}

void EditDeviceCommand::redo()
{
    apply(m_newLabel, m_newPorts);
}

void EditDeviceCommand::undo()
{
    apply(m_oldLabel, m_oldPorts);
}

// ---- EditDocumentInfoCommand ------------------------------------------------

EditDocumentInfoCommand::EditDocumentInfoCommand(StageScene *scene,
                                                 const DocumentInfo &oldInfo,
                                                 const DocumentInfo &newInfo)
    : m_scene(scene)
    , m_oldInfo(oldInfo)
    , m_newInfo(newInfo)
{
    setText(QObject::tr("Edit Document Info"));
}

void EditDocumentInfoCommand::redo()
{
    m_scene->setDocumentInfo(m_newInfo);
}

void EditDocumentInfoCommand::undo()
{
    m_scene->setDocumentInfo(m_oldInfo);
}

// ---- ChangePageConfigCommand ------------------------------------------------

ChangePageConfigCommand::ChangePageConfigCommand(StageScene *scene,
                                                 const PageConfig &oldConfig,
                                                 const PageConfig &newConfig)
    : m_scene(scene)
    , m_oldConfig(oldConfig)
    , m_newConfig(newConfig)
{
    setText(QObject::tr("Change Page Setup"));
}

void ChangePageConfigCommand::redo()
{
    m_scene->setPageConfig(m_newConfig);
}

void ChangePageConfigCommand::undo()
{
    m_scene->setPageConfig(m_oldConfig);
}