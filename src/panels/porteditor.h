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

#ifndef PORTEDITOR_H
#define PORTEDITOR_H

#include "ports.h"

#include <QList>
#include <QWidget>

class DeviceItem;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;

// Properties panel for the selected device: edits its label and its ports
// (connector / level / signal / balance / phantom / provided-by / to-console /
// notes), plus add/remove ports. Emits edited() so the scene can renumber.
class PortEditor : public QWidget
{
    Q_OBJECT

public:
    explicit PortEditor(QWidget *parent = nullptr);

    // Bind to a device (null clears the panel).
    void setDevice(DeviceItem *item);

    // Hide the built-in label row (the object editor supplies its own name field).
    void setLabelRowVisible(bool visible);

signals:
    void edited();

private:
    enum EditCol {
        EcLabel = 0,
        EcDirection,
        EcConnector,
        EcLevel,
        EcSignal,
        EcBalanced,
        EcPhantom,
        EcConsole,
        EcProvided,
        EcNotes,
        EcCount
    };

    void rebuild();
    void writeBack();
    void addPort();
    void removePort();
    QList<Port> collectPorts() const;

    DeviceItem *m_item = nullptr;
    bool m_building = false;

    QStackedWidget *m_stack = nullptr;
    QWidget *m_labelRow = nullptr;
    QLineEdit *m_labelEdit = nullptr;
    QTableWidget *m_table = nullptr;
    QPushButton *m_removeButton = nullptr;
};

#endif // PORTEDITOR_H