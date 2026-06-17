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

#include "inputlist.h"

#include <QRectF>
#include <QtTest>

class TestInputList : public QObject
{
    Q_OBJECT

private slots:
    void lineMono();
    void lineStereoWithPhantom();
    void columnsByOrientation();
    void measureModeCapacity();
};

void TestInputList::lineMono()
{
    InputListEntry e;
    e.numbers = QStringLiteral("3");
    e.signalWord = QStringLiteral("Mono");
    e.connector = QStringLiteral("XLR");
    e.level = QStringLiteral("Mic");
    e.phantom = false;

    QCOMPARE(inputlist::line(e, false),
             QStringLiteral("3. Mono XLR with Mic Level output"));
    // phantom flag clear → +48V never appended, even when showing is enabled.
    QCOMPARE(inputlist::line(e, true),
             QStringLiteral("3. Mono XLR with Mic Level output"));
}

void TestInputList::lineStereoWithPhantom()
{
    InputListEntry e;
    e.numbers = QStringLiteral("1–2");  // "1–2"
    e.signalWord = QStringLiteral("Stereo");
    e.connector = QStringLiteral("TS");
    e.level = QStringLiteral("Line");
    e.phantom = true;

    QCOMPARE(inputlist::line(e, false),
             QStringLiteral("1–2. Stereo TS with Line Level output"));
    QCOMPARE(inputlist::line(e, true),
             QStringLiteral("1–2. Stereo TS with Line Level output +48V"));
}

void TestInputList::columnsByOrientation()
{
    QCOMPARE(inputlist::columnsFor(QPageLayout::Portrait), 2);
    QCOMPARE(inputlist::columnsFor(QPageLayout::Landscape), 4);
}

void TestInputList::measureModeCapacity()
{
    QList<InputListEntry> entries;
    for (int i = 0; i < 10; ++i) {
        InputListEntry e;
        e.numbers = QString::number(i + 1);
        e.signalWord = QStringLiteral("Mono");
        e.connector = QStringLiteral("XLR");
        e.level = QStringLiteral("Mic");
        entries.append(e);
    }

    // 200px tall / 20px rows = 10 rows per column * 2 columns = 20 slots: all fit.
    QCOMPARE(inputlist::draw(nullptr, QRectF(0, 0, 400, 200), entries, 2, 20.0, false, 0),
             10);
    // 60px tall / 20px rows = 3 rows per column * 2 columns = 6 slots: 6 fit.
    QCOMPARE(inputlist::draw(nullptr, QRectF(0, 0, 400, 60), entries, 2, 20.0, false, 0),
             6);
}

QTEST_APPLESS_MAIN(TestInputList)
#include "test_inputlist.moc"