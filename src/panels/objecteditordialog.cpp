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

#include "objecteditordialog.h"

#include "deviceitem.h"
#include "porteditor.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

ObjectEditorDialog::ObjectEditorDialog(const DeviceType &initial,
                                       const QStringList &categories, QWidget *parent)
    : QDialog(parent)
    , m_id(initial.id)
    , m_icon(initial.icon)
{
    setWindowTitle(tr("Edit Object"));

    m_name = new QLineEdit(initial.name);
    m_name->setPlaceholderText(tr("Object name"));

    m_category = new QComboBox;
    m_category->setEditable(true);          // pick an existing one or type a new one
    m_category->addItems(categories);
    m_category->setCurrentText(initial.category);

    m_width = new QDoubleSpinBox;
    m_height = new QDoubleSpinBox;
    for (QDoubleSpinBox *spin : {m_width, m_height}) {
        spin->setRange(8.0, 400.0);
        spin->setSuffix(tr(" px"));
        spin->setDecimals(0);
    }
    m_width->setValue(initial.defaultSize.width());
    m_height->setValue(initial.defaultSize.height());

    m_iconPreview = new QLabel;
    m_iconPreview->setFixedSize(64, 64);
    m_iconPreview->setAlignment(Qt::AlignCenter);
    m_iconPreview->setFrameShape(QFrame::StyledPanel);
    auto *chooseIcon = new QPushButton(tr("Choose…"));
    connect(chooseIcon, &QPushButton::clicked, this, &ObjectEditorDialog::chooseIcon);
    auto *iconRow = new QHBoxLayout;
    iconRow->addWidget(m_iconPreview);
    iconRow->addWidget(chooseIcon);
    iconRow->addStretch();

    auto *form = new QFormLayout;
    form->addRow(tr("Name:"), m_name);
    form->addRow(tr("Category:"), m_category);
    auto *sizeRow = new QHBoxLayout;
    sizeRow->addWidget(m_width);
    sizeRow->addWidget(new QLabel(QStringLiteral("×")));
    sizeRow->addWidget(m_height);
    sizeRow->addStretch();
    form->addRow(tr("Size:"), sizeRow);
    form->addRow(tr("Icon:"), iconRow);

    // Reuse PortEditor for the ports, bound to a throwaway device. Its own label
    // row is hidden — the name is the field above.
    m_work = new DeviceItem(initial);
    m_portEditor = new PortEditor;
    m_portEditor->setLabelRowVisible(false);
    m_portEditor->setDevice(m_work);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &ObjectEditorDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(new QLabel(tr("Ports:")));
    layout->addWidget(m_portEditor, 1);
    layout->addWidget(buttons);
    resize(560, 560);

    updateIconPreview();
}

ObjectEditorDialog::~ObjectEditorDialog()
{
    delete m_work;  // transient, has no QObject parent
}

void ObjectEditorDialog::chooseIcon()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Choose Icon"), QString(),
        tr("Images (*.svg *.png *.jpg *.jpeg)"));
    if (path.isEmpty())
        return;
    DeviceIcon icon = DeviceIcon::fromPath(path);
    if (!icon.isValid()) {
        QMessageBox::warning(this, tr("Icon"), tr("Could not load that image."));
        return;
    }
    m_icon = icon;
    updateIconPreview();
}

void ObjectEditorDialog::updateIconPreview()
{
    if (m_icon.isValid())
        m_iconPreview->setPixmap(m_icon.toIcon().pixmap(56, 56));
    else
        m_iconPreview->setText(tr("(none)"));
}

void ObjectEditorDialog::accept()
{
    if (m_name->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Object"), tr("Please give the object a name."));
        return;
    }
    if (m_category->currentText().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Object"), tr("Please choose or enter a category."));
        return;
    }
    if (!m_icon.isValid()) {
        QMessageBox::warning(this, tr("Object"), tr("Please choose an icon."));
        return;
    }
    QDialog::accept();
}

DeviceType ObjectEditorDialog::object() const
{
    DeviceType type;
    type.id = m_id;
    type.name = m_name->text().trimmed();
    type.category = m_category->currentText().trimmed();
    type.icon = m_icon;
    type.defaultSize = QSizeF(m_width->value(), m_height->value());
    type.ports = m_work->ports();
    type.builtin = false;
    return type;
}