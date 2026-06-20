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
#include "undocommands.h"

#include <QFile>
#include <QUndoStack>
#include <QtTest>

namespace {
const char *const kMic = "mic-straight-overhead";
const char *const kSynth = "synth-61-overhead";  // stereo output

DeviceCatalog loadCatalog()
{
    DeviceCatalog catalog;
    QFile file(QStringLiteral(STAGEPLT_ASSETS_DIR "/catalog.json"));
    if (file.open(QIODevice::ReadOnly))
        catalog.loadFromJson(file.readAll(), QStringLiteral(STAGEPLT_ASSETS_DIR));
    return catalog;
}

int deviceCount(const StageScene &scene)
{
    int n = 0;
    for (QGraphicsItem *gi : scene.items())
        if (gi->type() == DeviceItem::Type)
            ++n;
    return n;
}
}  // namespace

class TestUndo : public QObject
{
    Q_OBJECT

private:
    DeviceCatalog m_catalog;

private slots:
    void initTestCase() { m_catalog = loadCatalog(); }

    void addUndoRedo();
    void removeUndoRedo();
    void transformUndoRedo();
    void editDeviceUndoRedo();
    void labelOffsetUndoRedo();
    void documentInfoUndoRedo();
    void pageConfigUndoRedo();
    void cleanState();
    void interleavedAddMoveDelete();
};

void TestUndo::addUndoRedo()
{
    StageScene scene(&m_catalog);
    QUndoStack stack;
    stack.push(new AddDeviceCommand(&scene, QString::fromLatin1(kMic), QPointF(100, 300)));
    QCOMPARE(deviceCount(scene), 1);
    QCOMPARE(scene.channels().size(), 1);

    stack.undo();
    QCOMPARE(deviceCount(scene), 0);
    QCOMPARE(scene.channels().size(), 0);

    stack.redo();
    QCOMPARE(deviceCount(scene), 1);
    QCOMPARE(scene.channels().size(), 1);
}

void TestUndo::removeUndoRedo()
{
    StageScene scene(&m_catalog);
    QUndoStack stack;
    scene.addDevice(QString::fromLatin1(kMic), QPointF(100, 300));
    auto *second = scene.addDevice(QString::fromLatin1(kMic), QPointF(200, 300));
    QCOMPARE(deviceCount(scene), 2);

    stack.push(new RemoveDevicesCommand(&scene, {second}));
    QCOMPARE(deviceCount(scene), 1);
    stack.undo();
    QCOMPARE(deviceCount(scene), 2);
    stack.redo();
    QCOMPARE(deviceCount(scene), 1);
}

void TestUndo::transformUndoRedo()
{
    StageScene scene(&m_catalog);
    QUndoStack stack;
    auto *item = scene.addDevice(QString::fromLatin1(kMic), QPointF(100, 300));
    const QPointF oldPos = item->pos();

    const QPointF newPos = oldPos + QPointF(50, 0);
    DeviceTransform t;
    t.item = item;
    t.oldPos = oldPos;
    t.newPos = newPos;
    t.newRotation = 90.0;
    QVector<DeviceTransform> changes{t};
    stack.push(new TransformDevicesCommand(changes, QStringLiteral("Move")));
    QCOMPARE(item->rotation(), 90.0);
    QCOMPARE(item->pos(), newPos);

    stack.undo();
    QCOMPARE(item->rotation(), 0.0);
    QCOMPARE(item->pos(), oldPos);

    stack.redo();
    QCOMPARE(item->rotation(), 90.0);
}

void TestUndo::editDeviceUndoRedo()
{
    StageScene scene(&m_catalog);
    QUndoStack stack;
    auto *item = scene.addDevice(QString::fromLatin1(kMic), QPointF(100, 300));
    const QString oldLabel = item->label();
    const QList<Port> oldPorts = item->ports();

    QList<Port> newPorts = oldPorts;
    newPorts.first().level = SignalLevel::Line;  // was Mic
    stack.push(new EditDeviceCommand(&scene, item, oldLabel, oldPorts,
                                     QStringLiteral("Lead Vox"), newPorts));
    QCOMPARE(item->label(), QStringLiteral("Lead Vox"));
    QCOMPARE(int(item->ports().first().level), int(SignalLevel::Line));

    stack.undo();
    QCOMPARE(item->label(), oldLabel);
    QCOMPARE(int(item->ports().first().level), int(SignalLevel::Mic));
}

void TestUndo::labelOffsetUndoRedo()
{
    StageScene scene(&m_catalog);
    QUndoStack stack;
    auto *item = scene.addDevice(QString::fromLatin1(kMic), QPointF(100, 300));
    QVERIFY(item->labelOffset().isNull());

    const QPointF off(20, -15);
    DeviceTransform t;
    t.item = item;
    t.oldPos = t.newPos = item->pos();
    t.oldRotation = t.newRotation = item->rotation();
    t.newLabelOffset = off;
    stack.push(new TransformDevicesCommand({t}, QStringLiteral("Move Label")));
    QCOMPARE(item->labelOffset(), off);

    stack.undo();
    QVERIFY(item->labelOffset().isNull());
    stack.redo();
    QCOMPARE(item->labelOffset(), off);
}

void TestUndo::documentInfoUndoRedo()
{
    StageScene scene(&m_catalog);
    QUndoStack stack;
    const DocumentInfo oldInfo = scene.documentInfo();
    DocumentInfo newInfo = oldInfo;
    newInfo.bandName = QStringLiteral("Test Band");

    stack.push(new EditDocumentInfoCommand(&scene, oldInfo, newInfo));
    QCOMPARE(scene.documentInfo().bandName, QStringLiteral("Test Band"));
    stack.undo();
    QVERIFY(scene.documentInfo().bandName.isEmpty());
    stack.redo();
    QCOMPARE(scene.documentInfo().bandName, QStringLiteral("Test Band"));
}

void TestUndo::pageConfigUndoRedo()
{
    StageScene scene(&m_catalog);
    QUndoStack stack;
    const PageConfig oldCfg = scene.pageConfig();
    const PageConfig newCfg{QPageSize::A4, QPageLayout::Landscape};

    stack.push(new ChangePageConfigCommand(&scene, oldCfg, newCfg));
    QVERIFY(scene.pageConfig() == newCfg);
    stack.undo();
    QVERIFY(scene.pageConfig() == oldCfg);
    stack.redo();
    QVERIFY(scene.pageConfig() == newCfg);
}

void TestUndo::cleanState()
{
    StageScene scene(&m_catalog);
    QUndoStack stack;
    QVERIFY(stack.isClean());
    stack.push(new AddDeviceCommand(&scene, QString::fromLatin1(kMic), QPointF(100, 300)));
    QVERIFY(!stack.isClean());
    stack.setClean();
    QVERIFY(stack.isClean());
    stack.undo();
    QVERIFY(!stack.isClean());  // moved away from the saved state
    stack.redo();
    QVERIFY(stack.isClean());
}

void TestUndo::interleavedAddMoveDelete()
{
    // Pointer-stability: a device referenced by a move command is later deleted
    // and that delete is undone (re-inserting the same object); undoing the move
    // must still resolve to a live item.
    StageScene scene(&m_catalog);
    QUndoStack stack;

    stack.push(new AddDeviceCommand(&scene, QString::fromLatin1(kSynth), QPointF(100, 300)));
    auto *item = static_cast<DeviceItem *>(
        [&] {
            for (QGraphicsItem *gi : scene.items())
                if (gi->type() == DeviceItem::Type)
                    return gi;
            return static_cast<QGraphicsItem *>(nullptr);
        }());
    QVERIFY(item);
    const QPointF p0 = item->pos();

    DeviceTransform mv;
    mv.item = item;
    mv.oldPos = p0;
    mv.newPos = p0 + QPointF(40, 0);
    mv.newRotation = 30.0;
    stack.push(new TransformDevicesCommand({mv}, QStringLiteral("Move")));
    stack.push(new RemoveDevicesCommand(&scene, {item}));
    QCOMPARE(deviceCount(scene), 0);

    stack.undo();  // undo remove -> same object back in scene
    QCOMPARE(deviceCount(scene), 1);
    stack.undo();  // undo move -> reverts transform on the live item
    QCOMPARE(item->pos(), p0);
    QCOMPARE(item->rotation(), 0.0);
    stack.undo();  // undo add -> empty
    QCOMPARE(deviceCount(scene), 0);

    stack.redo();  // add
    stack.redo();  // move
    stack.redo();  // remove
    QCOMPARE(deviceCount(scene), 0);
}

QTEST_MAIN(TestUndo)
#include "test_undo.moc"