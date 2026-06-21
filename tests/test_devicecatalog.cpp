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
#include "deviceicon.h"
#include "devicetype.h"

#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

namespace {
QByteArray realCatalogJson()
{
    QFile file(QStringLiteral(STAGEPLT_ASSETS_DIR "/catalog.json"));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}
}  // namespace

class TestDeviceCatalog : public QObject
{
    Q_OBJECT

private slots:
    void loadsRealCatalog();
    void userLibraryRoundTrip();
    void importsObjectPack();
    void removesObjectPack();
    void embeddedObjectRoundTrip();
    void categoriesRemapped();
    void orderedCategoriesMatchDeclaredOrder();
    void appendsUnlistedCategory();
};

void TestDeviceCatalog::loadsRealCatalog()
{
    DeviceCatalog catalog;
    QString error;
    QVERIFY2(catalog.loadFromJson(realCatalogJson(),
                                  QStringLiteral(STAGEPLT_ASSETS_DIR), &error),
             qPrintable(error));
    QCOMPARE(catalog.devices().size(), 16);

    const DeviceType *synth = catalog.find(QStringLiteral("synth-61-overhead"));
    QVERIFY(synth != nullptr);
    QCOMPARE(synth->category, QStringLiteral("Keyboards"));
    QVERIFY(catalog.find(QStringLiteral("does-not-exist")) == nullptr);
}

void TestDeviceCatalog::userLibraryRoundTrip()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    DeviceCatalog catalog;
    QVERIFY(catalog.loadFromJson(realCatalogJson(), QStringLiteral(STAGEPLT_ASSETS_DIR)));
    catalog.setUserLibraryPath(tmp.path());
    const int builtinCount = catalog.devices().size();

    DeviceType obj;
    obj.id = QStringLiteral("user-thing");
    obj.name = QStringLiteral("My Thing");
    obj.category = QStringLiteral("Lighting");  // a brand-new category
    obj.icon = DeviceIcon::fromPath(
        QStringLiteral(STAGEPLT_ASSETS_DIR "/mic-straight-overhead.svg"));
    obj.defaultSize = QSizeF(60, 40);           // non-square, allowed for custom

    QString error;
    QVERIFY2(catalog.addUserObject(obj, &error), qPrintable(error));
    QCOMPARE(catalog.devices().size(), builtinCount + 1);
    QVERIFY(catalog.isUserObject(QStringLiteral("user-thing")));
    QVERIFY(!catalog.isUserObject(QStringLiteral("mic-straight-overhead")));  // built-in

    // Reload from disk into a fresh catalog.
    DeviceCatalog reloaded;
    QVERIFY(reloaded.loadFromJson(realCatalogJson(), QStringLiteral(STAGEPLT_ASSETS_DIR)));
    reloaded.setUserLibraryPath(tmp.path());
    QVERIFY(reloaded.loadUserLibrary());

    const DeviceType *restored = reloaded.find(QStringLiteral("user-thing"));
    QVERIFY(restored != nullptr);
    QCOMPARE(restored->name, QStringLiteral("My Thing"));
    QVERIFY(!restored->builtin);
    QVERIFY(restored->icon.isValid());
    const QSizeF expectedSize(60, 40);
    QCOMPARE(restored->defaultSize, expectedSize);
    QVERIFY(reloaded.orderedCategories().contains(QStringLiteral("Lighting")));

    QVERIFY(reloaded.removeUserObject(QStringLiteral("user-thing")));
    QVERIFY(reloaded.find(QStringLiteral("user-thing")) == nullptr);
}

void TestDeviceCatalog::importsObjectPack()
{
    // Build a pack the way figures/generate.py --emit packs does: an objects.json
    // (user-library shape) plus sibling icon files referenced by filename.
    QTemporaryDir pack;
    QVERIFY(pack.isValid());
    QVERIFY(QFile::copy(QStringLiteral(STAGEPLT_ASSETS_DIR "/mic-straight-overhead.svg"),
                        QDir(pack.path()).filePath(QStringLiteral("fig.svg"))));
    {
        const QByteArray objects = R"({
            "version": 1,
            "name": "Test Pack",
            "devices": [
                {"id": "fig-x", "name": "Figure X", "category": "People",
                 "icon": "fig.svg", "defaultSize": [112, 116], "ports": []},
                {"id": "fig-missing", "name": "Missing Icon", "category": "People",
                 "icon": "absent.svg", "defaultSize": [40, 40], "ports": []}
            ]
        })";
        QFile f(QDir(pack.path()).filePath(QStringLiteral("objects.json")));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(objects);
    }

    QTemporaryDir lib;
    QVERIFY(lib.isValid());
    DeviceCatalog catalog;
    QVERIFY(catalog.loadFromJson(realCatalogJson(), QStringLiteral(STAGEPLT_ASSETS_DIR)));
    catalog.setUserLibraryPath(lib.path());

    QStringList added;
    QString error;
    const int count = catalog.importPack(
        QDir(pack.path()).filePath(QStringLiteral("objects.json")), &added, &error);
    QCOMPARE(count, 1);  // the entry with the missing icon is skipped
    QCOMPARE(added, QStringList{QStringLiteral("Figure X")});

    const DeviceType *fig = catalog.find(QStringLiteral("fig-x"));
    QVERIFY(fig != nullptr);
    QVERIFY(!fig->builtin);
    QVERIFY(fig->icon.isValid());
    const QSizeF expected(112, 116);
    QCOMPARE(fig->defaultSize, expected);
    QVERIFY(catalog.find(QStringLiteral("fig-missing")) == nullptr);

    // It persisted to the user library: a fresh catalog reloads it from disk.
    DeviceCatalog reloaded;
    QVERIFY(reloaded.loadFromJson(realCatalogJson(), QStringLiteral(STAGEPLT_ASSETS_DIR)));
    reloaded.setUserLibraryPath(lib.path());
    QVERIFY(reloaded.loadUserLibrary());
    QVERIFY(reloaded.isUserObject(QStringLiteral("fig-x")));

    // A non-existent pack is a hard error, not a silent zero.
    QCOMPARE(catalog.importPack(QStringLiteral("/no/such/objects.json")), -1);
}

void TestDeviceCatalog::removesObjectPack()
{
    // A pack with two objects, plus a hand-made (untagged) user object.
    QTemporaryDir pack;
    QVERIFY(pack.isValid());
    QVERIFY(QFile::copy(QStringLiteral(STAGEPLT_ASSETS_DIR "/mic-straight-overhead.svg"),
                        QDir(pack.path()).filePath(QStringLiteral("fig.svg"))));
    {
        const QByteArray objects = R"({
            "version": 1, "name": "My Pack",
            "devices": [
                {"id": "p1", "name": "P1", "category": "People", "icon": "fig.svg", "ports": []},
                {"id": "p2", "name": "P2", "category": "People", "icon": "fig.svg", "ports": []}
            ]
        })";
        QFile f(QDir(pack.path()).filePath(QStringLiteral("objects.json")));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(objects);
    }

    QTemporaryDir lib;
    QVERIFY(lib.isValid());
    DeviceCatalog catalog;
    QVERIFY(catalog.loadFromJson(realCatalogJson(), QStringLiteral(STAGEPLT_ASSETS_DIR)));
    catalog.setUserLibraryPath(lib.path());

    QCOMPARE(catalog.importPack(QDir(pack.path()).filePath(QStringLiteral("objects.json"))), 2);

    DeviceType handmade;  // an untagged user object
    handmade.id = QStringLiteral("hand");
    handmade.name = QStringLiteral("Hand Made");
    handmade.category = QStringLiteral("Amplifiers");
    handmade.icon = DeviceIcon::fromPath(
        QStringLiteral(STAGEPLT_ASSETS_DIR "/mic-straight-overhead.svg"));
    QVERIFY(catalog.addUserObject(handmade));

    // The pack tag persisted to disk and groups correctly.
    QCOMPARE(catalog.find(QStringLiteral("p1"))->pack, QStringLiteral("My Pack"));
    QVERIFY(catalog.find(QStringLiteral("hand"))->pack.isEmpty());
    const auto packs = catalog.importedPacks();
    QCOMPARE(packs.size(), 2);  // "My Pack" + the ungrouped bucket
    int tagged = 0, ungrouped = 0;
    for (const auto &p : packs) {
        if (p.name == QStringLiteral("My Pack")) { tagged = p.count; }
        else if (p.name.isEmpty()) { ungrouped = p.count; }
    }
    QCOMPARE(tagged, 2);
    QCOMPARE(ungrouped, 1);

    // Removing the pack drops its objects and their icon files, but not the
    // hand-made one nor any built-in.
    QCOMPARE(catalog.removePack(QStringLiteral("My Pack")), 2);
    QVERIFY(catalog.find(QStringLiteral("p1")) == nullptr);
    QVERIFY(catalog.find(QStringLiteral("p2")) == nullptr);
    QVERIFY(catalog.find(QStringLiteral("hand")) != nullptr);
    QVERIFY(!QFile::exists(QDir(lib.path()).filePath(QStringLiteral("p1.svg"))));

    // Removing the ungrouped bucket clears the hand-made object; reload confirms.
    QCOMPARE(catalog.removePack(QString()), 1);
    QVERIFY(catalog.find(QStringLiteral("hand")) == nullptr);

    DeviceCatalog reloaded;
    QVERIFY(reloaded.loadFromJson(realCatalogJson(), QStringLiteral(STAGEPLT_ASSETS_DIR)));
    reloaded.setUserLibraryPath(lib.path());
    QVERIFY(reloaded.loadUserLibrary());
    QVERIFY(reloaded.importedPacks().isEmpty());  // nothing user-made remains
}

void TestDeviceCatalog::embeddedObjectRoundTrip()
{
    DeviceType obj;
    obj.id = QStringLiteral("emb");
    obj.name = QStringLiteral("Embedded");
    obj.category = QStringLiteral("DJ");
    obj.defaultSize = QSizeF(50, 30);
    obj.icon = DeviceIcon::fromPath(
        QStringLiteral(STAGEPLT_ASSETS_DIR "/mic-straight-overhead.svg"));
    Port port;
    port.label = QStringLiteral("Out");
    obj.ports.append(port);

    const DeviceType r = DeviceCatalog::fromEmbeddedJson(DeviceCatalog::toEmbeddedJson(obj));
    QCOMPARE(r.id, QStringLiteral("emb"));
    QCOMPARE(r.name, QStringLiteral("Embedded"));
    QCOMPARE(r.category, QStringLiteral("DJ"));
    const QSizeF expectedSize(50, 30);
    QCOMPARE(r.defaultSize, expectedSize);
    QVERIFY(!r.builtin);
    QCOMPARE(r.ports.size(), 1);
    QVERIFY(r.icon.isSvg());
    QCOMPARE(r.icon.data(), obj.icon.data());  // icon bytes travel with the file
}

void TestDeviceCatalog::categoriesRemapped()
{
    DeviceCatalog catalog;
    catalog.loadFromJson(realCatalogJson(), QStringLiteral(STAGEPLT_ASSETS_DIR));
    const QStringList cats = catalog.categories();

    // Old labels are gone; the Phase-2 names are present.
    QVERIFY(!cats.contains(QStringLiteral("Keys")));
    QVERIFY(!cats.contains(QStringLiteral("Drums")));
    QVERIFY(!cats.contains(QStringLiteral("Mics")));
    QVERIFY(cats.contains(QStringLiteral("Keyboards")));
    QVERIFY(cats.contains(QStringLiteral("Microphones")));
    QVERIFY(cats.contains(QStringLiteral("Drums and Percussion")));
}

void TestDeviceCatalog::orderedCategoriesMatchDeclaredOrder()
{
    DeviceCatalog catalog;
    catalog.loadFromJson(realCatalogJson(), QStringLiteral(STAGEPLT_ASSETS_DIR));
    const QStringList expected = {
        QStringLiteral("Keyboards"),       QStringLiteral("Amplifiers"),
        QStringLiteral("Microphones"),     QStringLiteral("Drums and Percussion"),
        QStringLiteral("Monitors"),        QStringLiteral("Racks"),
        QStringLiteral("Effects"),         QStringLiteral("People"),
        QStringLiteral("DJ")};
    QCOMPARE(catalog.orderedCategories(), expected);  // empty People/DJ still appear
}

void TestDeviceCatalog::appendsUnlistedCategory()
{
    // A device whose category isn't in "categories" must still surface, appended
    // after the declared order — nothing silently hidden from the palette.
    const QByteArray json = R"({
        "categories": ["Keyboards"],
        "devices": [
            {"id": "a", "name": "A", "category": "Keyboards", "icon": "a.svg", "ports": []},
            {"id": "b", "name": "B", "category": "Surprise",  "icon": "b.svg", "ports": []}
        ]
    })";
    DeviceCatalog catalog;
    QVERIFY(catalog.loadFromJson(json, QStringLiteral("/tmp")));
    const QStringList ordered = catalog.orderedCategories();
    QCOMPARE(ordered.size(), 2);
    QCOMPARE(ordered.first(), QStringLiteral("Keyboards"));
    QCOMPARE(ordered.last(), QStringLiteral("Surprise"));
}

QTEST_APPLESS_MAIN(TestDeviceCatalog)
#include "test_devicecatalog.moc"