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

class QJsonObject;

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

    // Palette sections in display order: the catalog's "categories" array
    // (which may include sections that have no devices yet), with any leftover
    // device categories appended. Falls back to categories() when unspecified.
    QStringList orderedCategories() const;

    bool isEmpty() const { return m_devices.isEmpty(); }
    bool isUserObject(const QString &id) const;  // user-created (editable) vs built-in

    // The writable user library: a directory holding objects.json + icon files.
    // Built-ins and user objects are merged into one catalog so the scene can
    // find() either. Mutations persist to disk immediately.
    void setUserLibraryPath(const QString &dir);
    QString userLibraryPath() const { return m_userDir; }
    bool loadUserLibrary(QString *error = nullptr);
    bool addUserObject(const DeviceType &type, QString *error = nullptr);
    bool removeUserObject(const QString &id, QString *error = nullptr);

    // Import an object pack — an objects.json (same shape as the user library)
    // with sibling icon files — adding each object to the user library (icons
    // copied in, existing same-id user objects replaced). Returns the number
    // imported, or -1 on a read/parse error (*error set). addedNames, if given,
    // collects the display names of the objects that were imported.
    int importPack(const QString &objectsJsonPath, QStringList *addedNames = nullptr,
                   QString *error = nullptr);

    // A group of user objects sharing a `pack` tag (name empty = ungrouped).
    struct PackInfo {
        QString name;
        int count = 0;
    };
    // User objects grouped by pack, first-seen order (includes an empty-name
    // entry for untagged objects when any exist).
    QList<PackInfo> importedPacks() const;
    // Remove every user object tagged with `pack` (empty = ungrouped), deleting
    // their icons and persisting once. Returns the count removed, or -1 if the
    // library could not be saved (*error set).
    int removePack(const QString &pack, QString *error = nullptr);

    // Portability: a custom object embedded in a .splot (icon as base64 bytes).
    static QJsonObject toEmbeddedJson(const DeviceType &type);
    static DeviceType fromEmbeddedJson(const QJsonObject &obj);
    // Make an object available for this session without writing it to disk
    // (used for embedded objects until the user imports them).
    void addInMemoryObject(const DeviceType &type);

private:
    bool saveUserLibrary(QString *error);

    QList<DeviceType> m_devices;
    QStringList m_categoryOrder;  // from the catalog's "categories" array
    QString m_userDir;            // user-library directory (empty = disabled)
};

#endif // DEVICECATALOG_H