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

#ifndef DEVICELIST_H
#define DEVICELIST_H

#include <QListWidget>

struct DeviceType;

// A draggable icon grid of devices for one palette category. Drags carry the
// DeviceType id (kDeviceMimeType); activating an item emits deviceActivated().
// DevicePalette stacks one of these per category inside an accordion.
class DeviceList : public QListWidget
{
    Q_OBJECT

public:
    explicit DeviceList(QWidget *parent = nullptr);

    void addDevice(const DeviceType &type);
    // A non-interactive, greyed hint shown in a category that has no devices yet.
    void addPlaceholder(const QString &text);

signals:
    void deviceActivated(const QString &typeId);

protected:
    QMimeData *mimeData(const QList<QListWidgetItem *> &items) const override;
    QStringList mimeTypes() const override;

private:
    static QString idOf(const QListWidgetItem *item);
};

#endif // DEVICELIST_H