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

#include "porteditor.h"

#include "deviceitem.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
QComboBox *makeCombo(const QStringList &items, int current)
{
    auto *combo = new QComboBox;
    combo->addItems(items);
    combo->setCurrentIndex(current);
    return combo;
}
}

PortEditor::PortEditor(QWidget *parent)
    : QWidget(parent)
{
    m_stack = new QStackedWidget(this);

    // Page 0: placeholder shown when nothing is selected.
    auto *placeholder = new QLabel(tr("Select a device to edit its ports."));
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setWordWrap(true);
    placeholder->setEnabled(false);
    m_stack->addWidget(placeholder);

    // Page 1: the editor.
    auto *editor = new QWidget;
    auto *layout = new QVBoxLayout(editor);

    m_labelRow = new QWidget;
    auto *form = new QFormLayout(m_labelRow);
    form->setContentsMargins(0, 0, 0, 0);
    m_labelEdit = new QLineEdit;
    form->addRow(tr("Label:"), m_labelEdit);
    layout->addWidget(m_labelRow);

    m_table = new QTableWidget(0, EcCount);
    m_table->setHorizontalHeaderLabels(
        {tr("Port"), tr("Dir"), tr("Connector"), tr("Level"), tr("Signal"),
         tr("Bal"), tr("+48V"), tr("To Console"), tr("Provided By"), tr("Notes")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_table);

    auto *buttons = new QHBoxLayout;
    auto *addButton = new QPushButton(tr("Add Port"));
    m_removeButton = new QPushButton(tr("Remove Port"));
    buttons->addWidget(addButton);
    buttons->addWidget(m_removeButton);
    buttons->addStretch();
    layout->addLayout(buttons);

    m_stack->addWidget(editor);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(m_stack);

    connect(m_labelEdit, &QLineEdit::textEdited, this, [this](const QString &text) {
        if (m_building || !m_item)
            return;
        m_item->setLabel(text);
        emit edited();
    });
    connect(m_table, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *) {
        writeBack();
    });
    connect(addButton, &QPushButton::clicked, this, &PortEditor::addPort);
    connect(m_removeButton, &QPushButton::clicked, this, &PortEditor::removePort);

    setDevice(nullptr);
}

void PortEditor::setDevice(DeviceItem *item)
{
    m_item = item;
    rebuild();
}

void PortEditor::setLabelRowVisible(bool visible)
{
    m_labelRow->setVisible(visible);
}

void PortEditor::rebuild()
{
    m_building = true;
    m_table->setRowCount(0);

    if (!m_item) {
        m_stack->setCurrentIndex(0);
        m_building = false;
        return;
    }
    m_stack->setCurrentIndex(1);
    m_labelEdit->setText(m_item->label());

    const QList<Port> &ps = m_item->ports();
    m_table->setRowCount(ps.size());
    for (int row = 0; row < ps.size(); ++row) {
        const Port &p = ps.at(row);

        m_table->setItem(row, EcLabel, new QTableWidgetItem(p.label));

        auto *dir = makeCombo(ports::directionDisplays(), static_cast<int>(p.direction));
        auto *conn = makeCombo(ports::connectorDisplays(), static_cast<int>(p.connector));
        auto *level = makeCombo(ports::levelDisplays(), static_cast<int>(p.level));
        auto *signal = makeCombo(ports::signalDisplays(), static_cast<int>(p.signal));
        auto *provided = makeCombo(ports::providedByDisplays(), static_cast<int>(p.providedBy));
        m_table->setCellWidget(row, EcDirection, dir);
        m_table->setCellWidget(row, EcConnector, conn);
        m_table->setCellWidget(row, EcLevel, level);
        m_table->setCellWidget(row, EcSignal, signal);
        m_table->setCellWidget(row, EcProvided, provided);

        auto *bal = new QCheckBox;
        bal->setChecked(p.balanced);
        auto *phantom = new QCheckBox;
        phantom->setChecked(p.phantom);
        auto *console = new QCheckBox;
        console->setChecked(p.toConsole);
        m_table->setCellWidget(row, EcBalanced, bal);
        m_table->setCellWidget(row, EcPhantom, phantom);
        m_table->setCellWidget(row, EcConsole, console);

        m_table->setItem(row, EcNotes, new QTableWidgetItem(p.notes));

        for (QComboBox *combo : {dir, conn, level, signal, provided})
            connect(combo, &QComboBox::currentIndexChanged, this, [this] { writeBack(); });
        for (QCheckBox *check : {bal, phantom, console})
            connect(check, &QCheckBox::toggled, this, [this] { writeBack(); });
    }
    m_table->resizeColumnsToContents();
    m_building = false;
}

QList<Port> PortEditor::collectPorts() const
{
    QList<Port> result;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        Port p;
        if (auto *labelItem = m_table->item(row, EcLabel))
            p.label = labelItem->text();
        if (auto *c = qobject_cast<QComboBox *>(m_table->cellWidget(row, EcDirection)))
            p.direction = static_cast<PortDirection>(c->currentIndex());
        if (auto *c = qobject_cast<QComboBox *>(m_table->cellWidget(row, EcConnector)))
            p.connector = static_cast<ConnectorType>(c->currentIndex());
        if (auto *c = qobject_cast<QComboBox *>(m_table->cellWidget(row, EcLevel)))
            p.level = static_cast<SignalLevel>(c->currentIndex());
        if (auto *c = qobject_cast<QComboBox *>(m_table->cellWidget(row, EcSignal)))
            p.signal = static_cast<SignalConfig>(c->currentIndex());
        if (auto *c = qobject_cast<QComboBox *>(m_table->cellWidget(row, EcProvided)))
            p.providedBy = static_cast<ProvidedBy>(c->currentIndex());
        if (auto *c = qobject_cast<QCheckBox *>(m_table->cellWidget(row, EcBalanced)))
            p.balanced = c->isChecked();
        if (auto *c = qobject_cast<QCheckBox *>(m_table->cellWidget(row, EcPhantom)))
            p.phantom = c->isChecked();
        if (auto *c = qobject_cast<QCheckBox *>(m_table->cellWidget(row, EcConsole)))
            p.toConsole = c->isChecked();
        if (auto *notesItem = m_table->item(row, EcNotes))
            p.notes = notesItem->text();
        result.append(p);
    }
    return result;
}

void PortEditor::writeBack()
{
    if (m_building || !m_item)
        return;
    m_item->setPorts(collectPorts());
    emit edited();
}

void PortEditor::addPort()
{
    if (!m_item)
        return;
    QList<Port> ps = m_item->ports();
    Port p;
    p.label = tr("Out");
    p.direction = PortDirection::Output;
    p.connector = ConnectorType::Xlr;
    p.level = SignalLevel::Line;
    ps.append(p);
    m_item->setPorts(ps);
    rebuild();
    emit edited();
}

void PortEditor::removePort()
{
    if (!m_item)
        return;
    const int row = m_table->currentRow();
    if (row < 0)
        return;
    QList<Port> ps = m_item->ports();
    if (row >= ps.size())
        return;
    ps.removeAt(row);
    m_item->setPorts(ps);
    rebuild();
    emit edited();
}