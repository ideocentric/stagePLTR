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

#include "devicecatalog.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>

bool DeviceCatalog::loadFromJson(const QByteArray &data,
                                const QString &iconResourceBase,
                                QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull()) {
        if (error)
            *error = QStringLiteral("JSON parse error: %1").arg(parseError.errorString());
        return false;
    }
    if (!doc.isObject()) {
        if (error)
            *error = QStringLiteral("Catalog root is not a JSON object");
        return false;
    }

    const QJsonArray devices = doc.object().value(QStringLiteral("devices")).toArray();
    QList<DeviceType> parsed;
    parsed.reserve(devices.size());

    for (const QJsonValue &value : devices) {
        const QJsonObject obj = value.toObject();
        const QString id = obj.value(QStringLiteral("id")).toString();
        const QString icon = obj.value(QStringLiteral("icon")).toString();
        if (id.isEmpty() || icon.isEmpty())
            continue;  // skip malformed entries rather than failing the whole load

        DeviceType type;
        type.id = id;
        type.name = obj.value(QStringLiteral("name")).toString(id);
        type.category = obj.value(QStringLiteral("category")).toString(QStringLiteral("Other"));
        type.iconResource = iconResourceBase + QLatin1Char('/') + icon;

        const QJsonArray size = obj.value(QStringLiteral("defaultSize")).toArray();
        if (size.size() == 2)
            type.defaultSize = QSizeF(size.at(0).toDouble(48.0), size.at(1).toDouble(48.0));

        const QJsonArray portArray = obj.value(QStringLiteral("ports")).toArray();
        for (const QJsonValue &portValue : portArray)
            type.ports.append(Port::fromJson(portValue.toObject()));

        parsed.append(type);
    }

    if (parsed.isEmpty()) {
        if (error)
            *error = QStringLiteral("Catalog contains no valid devices");
        return false;
    }

    m_devices = parsed;
    return true;
}

bool DeviceCatalog::loadFromResource(const QString &resourcePath,
                                    const QString &iconResourceBase,
                                    QString *error)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("Cannot open catalog resource: %1").arg(resourcePath);
        return false;
    }
    return loadFromJson(file.readAll(), iconResourceBase, error);
}

const DeviceType *DeviceCatalog::find(const QString &id) const
{
    for (const DeviceType &type : m_devices) {
        if (type.id == id)
            return &type;
    }
    return nullptr;
}

QStringList DeviceCatalog::categories() const
{
    QStringList result;
    QSet<QString> seen;
    for (const DeviceType &type : m_devices) {
        if (!seen.contains(type.category)) {
            seen.insert(type.category);
            result.append(type.category);
        }
    }
    return result;
}