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

#ifndef DEVICETYPE_H
#define DEVICETYPE_H

#include "deviceicon.h"
#include "ports.h"

#include <QList>
#include <QString>
#include <QSizeF>

// A catalog entry: one kind of device/instrument that can be placed on the stage.
// Loaded from the device catalog (see DeviceCatalog). `ports` is the default
// connection template copied onto each placed instance.
struct DeviceType
{
    QString id;            // stable identifier, e.g. "amp-small-overhead"
    QString name;          // display name, e.g. "Small Amp"
    QString category;      // palette grouping, e.g. "Amplifiers"
    DeviceIcon icon;       // symbol (SVG preferred; raster supported)
    QSizeF  defaultSize{48.0, 48.0};
    QList<Port> ports;     // default port template
    bool builtin = true;   // bundled (read-only) vs a user-created object
};

#endif // DEVICETYPE_H