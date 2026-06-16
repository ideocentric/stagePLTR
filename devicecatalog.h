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

#ifndef DEVICECATALOG_H
#define DEVICECATALOG_H

#include "devicetype.h"

#include <QList>
#include <QString>

// Loads and holds the set of available DeviceTypes from a JSON catalog.
//
// The catalog format is data-driven so the device library can grow without
// recompiling. loadFromJson() accepts raw bytes, so the same parser serves the
// built-in catalog (bundled at :/plot/catalog.json) and, later, user-supplied
// catalogs read from disk.
class DeviceCatalog
{
public:
    // Parse a catalog from raw JSON bytes. Icons are resolved against
    // iconResourceBase (e.g. ":/plot"). Returns false and sets *error on failure.
    bool loadFromJson(const QByteArray &data,
                      const QString &iconResourceBase,
                      QString *error = nullptr);

    // Convenience: load a catalog stored in the Qt resource system.
    bool loadFromResource(const QString &resourcePath,
                         const QString &iconResourceBase,
                         QString *error = nullptr);

    const QList<DeviceType> &devices() const { return m_devices; }
    const DeviceType *find(const QString &id) const;
    QStringList categories() const;  // unique, in first-seen order
    bool isEmpty() const { return m_devices.isEmpty(); }

private:
    QList<DeviceType> m_devices;
};

#endif // DEVICECATALOG_H