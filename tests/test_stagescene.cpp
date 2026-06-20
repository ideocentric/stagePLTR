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
#include "deviceitem.h"
#include "stagescene.h"

#include <QFile>
#include <QJsonObject>
#include <QtTest>

namespace {
DeviceCatalog loadCatalog()
{
    DeviceCatalog catalog;
    QFile file(QStringLiteral(STAGEPLT_ASSETS_DIR "/catalog.json"));
    if (file.open(QIODevice::ReadOnly))
        catalog.loadFromJson(file.readAll(), QStringLiteral(STAGEPLT_ASSETS_DIR));
    return catalog;
}
}  // namespace

class TestStageScene : public QObject
{
    Q_OBJECT

private slots:
    void emptySceneHasNoChannels();
    void monoOutputsNumberSequentially();
    void stereoOutputGroupsIntoRange();
    void labelOffsetPersists();
};

void TestStageScene::emptySceneHasNoChannels()
{
    DeviceCatalog catalog = loadCatalog();
    StageScene scene(&catalog);
    QVERIFY(scene.channels().isEmpty());
    QVERIFY(scene.inputListEntries().isEmpty());
}

void TestStageScene::monoOutputsNumberSequentially()
{
    DeviceCatalog catalog = loadCatalog();
    StageScene scene(&catalog);
    QVERIFY(scene.addDevice(QStringLiteral("mic-straight-overhead"), QPointF(100, 300)));
    QVERIFY(scene.addDevice(QStringLiteral("mic-boom-left-overhead"), QPointF(200, 300)));

    QCOMPARE(scene.channels().size(), 2);
    QCOMPARE(scene.channels().at(0).number, 1);
    QCOMPARE(scene.channels().at(1).number, 2);

    QCOMPARE(scene.inputListEntries().size(), 2);
    QCOMPARE(scene.inputListEntries().at(0).numbers, QStringLiteral("1"));
    QCOMPARE(scene.inputListEntries().at(0).signalWord, QStringLiteral("Mono"));
}

void TestStageScene::stereoOutputGroupsIntoRange()
{
    DeviceCatalog catalog = loadCatalog();
    StageScene scene(&catalog);
    // The 61-key synth has a single stereo output → two channel numbers, one
    // grouped legend entry.
    QVERIFY(scene.addDevice(QStringLiteral("synth-61-overhead"), QPointF(100, 300)));

    QCOMPARE(scene.channels().size(), 2);            // L + R
    QCOMPARE(scene.inputListEntries().size(), 1);    // grouped
    const InputListEntry entry = scene.inputListEntries().first();
    QCOMPARE(entry.numbers, QStringLiteral("1–2"));  // "1–2"
    QCOMPARE(entry.signalWord, QStringLiteral("Stereo"));
}

void TestStageScene::labelOffsetPersists()
{
    DeviceCatalog catalog = loadCatalog();
    StageScene scene(&catalog);
    auto *item = scene.addDevice(QStringLiteral("mic-straight-overhead"), QPointF(100, 300));
    const QPointF off(12, -8);
    item->setLabelOffset(off);

    StageScene reloaded(&catalog);
    reloaded.fromJson(scene.toJson());

    DeviceItem *restored = nullptr;
    for (QGraphicsItem *gi : reloaded.items())
        if (gi->type() == DeviceItem::Type)
            restored = static_cast<DeviceItem *>(gi);
    QVERIFY(restored);
    QCOMPARE(restored->labelOffset(), off);
}

QTEST_MAIN(TestStageScene)
#include "test_stagescene.moc"