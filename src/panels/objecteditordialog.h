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

#ifndef OBJECTEDITORDIALOG_H
#define OBJECTEDITORDIALOG_H

#include "deviceicon.h"
#include "devicetype.h"

#include <QDialog>
#include <QString>
#include <QStringList>

class DeviceItem;
class PortEditor;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;

// Edits a user object's template: name, category (pick existing or type a new
// one), default size (may be non-square), icon (SVG preferred, raster allowed),
// and its ports. Ports are edited by reusing PortEditor bound to a transient
// device. The object's id is fixed (carried from the source); on OK, object()
// returns the edited DeviceType (builtin=false).
class ObjectEditorDialog : public QDialog
{
    Q_OBJECT

public:
    ObjectEditorDialog(const DeviceType &initial, const QStringList &categories,
                       QWidget *parent = nullptr);
    ~ObjectEditorDialog() override;

    DeviceType object() const;

private:
    void chooseIcon();
    void updateIconPreview();
    void accept() override;

    QString m_id;
    DeviceIcon m_icon;

    QLineEdit *m_name = nullptr;
    QComboBox *m_category = nullptr;
    QDoubleSpinBox *m_width = nullptr;
    QDoubleSpinBox *m_height = nullptr;
    QLabel *m_iconPreview = nullptr;
    PortEditor *m_portEditor = nullptr;
    DeviceItem *m_work = nullptr;  // transient, owns the ports being edited
};

#endif // OBJECTEDITORDIALOG_H