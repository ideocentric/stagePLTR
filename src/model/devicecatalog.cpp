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

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>

#include <algorithm>
#include <utility>

namespace {

// Parse one catalog entry; icons resolve against iconBase (a resource root or a
// directory). Returns false if the entry is malformed (missing id/icon).
bool parseDeviceType(const QJsonObject &obj, const QString &iconBase, bool builtin,
                     DeviceType *out)
{
    const QString id = obj.value(QStringLiteral("id")).toString();
    const QString icon = obj.value(QStringLiteral("icon")).toString();
    if (id.isEmpty() || icon.isEmpty())
        return false;

    out->id = id;
    out->name = obj.value(QStringLiteral("name")).toString(id);
    out->category = obj.value(QStringLiteral("category")).toString(QStringLiteral("Other"));
    out->icon = DeviceIcon::fromPath(iconBase + QLatin1Char('/') + icon);
    out->builtin = builtin;

    const QJsonArray size = obj.value(QStringLiteral("defaultSize")).toArray();
    if (size.size() == 2)
        out->defaultSize = QSizeF(size.at(0).toDouble(48.0), size.at(1).toDouble(48.0));

    const QJsonArray portArray = obj.value(QStringLiteral("ports")).toArray();
    for (const QJsonValue &portValue : portArray)
        out->ports.append(Port::fromJson(portValue.toObject()));
    return true;
}

// File name for a user object's icon within the library directory.
QString iconFileName(const DeviceType &type)
{
    const QString ext = type.icon.format().isEmpty() ? QStringLiteral("svg")
                                                      : type.icon.format();
    return type.id + QLatin1Char('.') + ext;
}

QJsonObject deviceTypeToJson(const DeviceType &type)
{
    QJsonObject obj;
    obj[QStringLiteral("id")] = type.id;
    obj[QStringLiteral("name")] = type.name;
    obj[QStringLiteral("category")] = type.category;
    obj[QStringLiteral("icon")] = iconFileName(type);
    obj[QStringLiteral("defaultSize")] =
        QJsonArray{type.defaultSize.width(), type.defaultSize.height()};
    QJsonArray ports;
    for (const Port &port : type.ports)
        ports.append(port.toJson());
    obj[QStringLiteral("ports")] = ports;
    return obj;
}

}  // namespace

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

    QStringList categoryOrder;
    const QJsonArray categoryArray =
        doc.object().value(QStringLiteral("categories")).toArray();
    for (const QJsonValue &value : categoryArray) {
        const QString name = value.toString();
        if (!name.isEmpty())
            categoryOrder.append(name);
    }

    const QJsonArray devices = doc.object().value(QStringLiteral("devices")).toArray();
    QList<DeviceType> parsed;
    parsed.reserve(devices.size());

    for (const QJsonValue &value : devices) {
        DeviceType type;
        if (parseDeviceType(value.toObject(), iconResourceBase, /*builtin=*/true, &type))
            parsed.append(type);
    }

    if (parsed.isEmpty()) {
        if (error)
            *error = QStringLiteral("Catalog contains no valid devices");
        return false;
    }

    // Keep any already-loaded user objects; the built-ins replace prior built-ins.
    QList<DeviceType> userObjects;
    for (const DeviceType &t : std::as_const(m_devices))
        if (!t.builtin)
            userObjects.append(t);
    m_devices = parsed + userObjects;
    m_categoryOrder = categoryOrder;
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

QStringList DeviceCatalog::orderedCategories() const
{
    if (m_categoryOrder.isEmpty())
        return categories();  // no explicit order: fall back to first-seen

    // Start from the declared order, then append any device categories the
    // catalog forgot to list, so nothing is silently hidden from the palette.
    QStringList result = m_categoryOrder;
    QSet<QString> known(m_categoryOrder.begin(), m_categoryOrder.end());
    for (const DeviceType &type : m_devices) {
        if (!known.contains(type.category)) {
            known.insert(type.category);
            result.append(type.category);
        }
    }
    return result;
}

bool DeviceCatalog::isUserObject(const QString &id) const
{
    const DeviceType *type = find(id);
    return type && !type->builtin;
}

void DeviceCatalog::setUserLibraryPath(const QString &dir)
{
    m_userDir = dir;
}

bool DeviceCatalog::loadUserLibrary(QString *error)
{
    if (m_userDir.isEmpty())
        return true;
    const QString path = QDir(m_userDir).filePath(QStringLiteral("objects.json"));
    QFile file(path);
    if (!file.exists())
        return true;  // no library yet — that's fine
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("Cannot read user library: %1").arg(path);
        return false;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    const QJsonArray devices = doc.object().value(QStringLiteral("devices")).toArray();

    // Drop any previously-loaded user objects, then load fresh.
    m_devices.erase(std::remove_if(m_devices.begin(), m_devices.end(),
                                   [](const DeviceType &t) { return !t.builtin; }),
                    m_devices.end());
    for (const QJsonValue &value : devices) {
        DeviceType type;
        if (parseDeviceType(value.toObject(), m_userDir, /*builtin=*/false, &type))
            m_devices.append(type);
    }
    return true;
}

bool DeviceCatalog::saveUserLibrary(QString *error)
{
    if (m_userDir.isEmpty()) {
        if (error)
            *error = QStringLiteral("No user library location set");
        return false;
    }
    QDir dir(m_userDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (error)
            *error = QStringLiteral("Cannot create user library: %1").arg(m_userDir);
        return false;
    }

    QJsonArray arr;
    for (const DeviceType &t : std::as_const(m_devices))
        if (!t.builtin)
            arr.append(deviceTypeToJson(t));
    QJsonObject root;
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("devices")] = arr;

    QFile file(dir.filePath(QStringLiteral("objects.json")));
    if (!file.open(QIODevice::WriteOnly)) {
        if (error)
            *error = QStringLiteral("Cannot write user library");
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool DeviceCatalog::addUserObject(const DeviceType &type, QString *error)
{
    QDir dir(m_userDir);
    if (m_userDir.isEmpty() || (!dir.exists() && !dir.mkpath(QStringLiteral(".")))) {
        if (error)
            *error = QStringLiteral("Cannot create user library: %1").arg(m_userDir);
        return false;
    }

    DeviceType stored = type;
    stored.builtin = false;

    QFile icon(dir.filePath(iconFileName(stored)));
    if (!icon.open(QIODevice::WriteOnly)) {
        if (error)
            *error = QStringLiteral("Cannot write icon for %1").arg(stored.id);
        return false;
    }
    icon.write(stored.icon.data());
    icon.close();

    // Replace any existing user object with the same id.
    for (int i = 0; i < m_devices.size(); ++i) {
        if (m_devices.at(i).id == stored.id && !m_devices.at(i).builtin) {
            m_devices.removeAt(i);
            break;
        }
    }
    m_devices.append(stored);
    return saveUserLibrary(error);
}

bool DeviceCatalog::removeUserObject(const QString &id, QString *error)
{
    int index = -1;
    for (int i = 0; i < m_devices.size(); ++i) {
        if (m_devices.at(i).id == id && !m_devices.at(i).builtin) {
            index = i;
            break;
        }
    }
    if (index < 0) {
        if (error)
            *error = QStringLiteral("No user object '%1'").arg(id);
        return false;
    }
    QFile::remove(QDir(m_userDir).filePath(iconFileName(m_devices.at(index))));
    m_devices.removeAt(index);
    return saveUserLibrary(error);
}