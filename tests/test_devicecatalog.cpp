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
#include "devicetype.h"

#include <QFile>
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