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

#ifndef DEVICEPALETTE_H
#define DEVICEPALETTE_H

#include <QListWidget>

class DeviceCatalog;

// Palette of available devices, shown as draggable icons. Drags carry the
// DeviceType id (kDeviceMimeType); double-clicking emits deviceActivated() so
// the window can drop the device at the page centre.
class DevicePalette : public QListWidget
{
    Q_OBJECT

public:
    explicit DevicePalette(QWidget *parent = nullptr);

    void populate(const DeviceCatalog &catalog);

signals:
    void deviceActivated(const QString &typeId);

protected:
    QMimeData *mimeData(const QList<QListWidgetItem *> &items) const override;
    QStringList mimeTypes() const override;

private:
    static QString idOf(const QListWidgetItem *item);
};

#endif // DEVICEPALETTE_H