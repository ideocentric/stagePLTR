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

#include "ports.h"

#include <QtTest>

class TestPorts : public QObject
{
    Q_OBJECT

private slots:
    void connectorKeyRoundTrip();
    void levelKeyRoundTrip();
    void signalAndProvidedByRoundTrip();
    void displaysAreNonEmpty();
    void portJsonRoundTrip();
};

void TestPorts::connectorKeyRoundTrip()
{
    for (auto c : {ConnectorType::Xlr, ConnectorType::QuarterTs, ConnectorType::QuarterTrs,
                   ConnectorType::Speakon, ConnectorType::Rca, ConnectorType::Other})
        QCOMPARE(int(ports::connectorFromKey(ports::toKey(c))), int(c));
}

void TestPorts::levelKeyRoundTrip()
{
    for (auto l : {SignalLevel::Mic, SignalLevel::Instrument, SignalLevel::Line,
                   SignalLevel::Speaker})
        QCOMPARE(int(ports::levelFromKey(ports::toKey(l))), int(l));
}

void TestPorts::signalAndProvidedByRoundTrip()
{
    for (auto s : {SignalConfig::Mono, SignalConfig::Stereo})
        QCOMPARE(int(ports::signalFromKey(ports::toKey(s))), int(s));
    for (auto p : {ProvidedBy::Unspecified, ProvidedBy::Band, ProvidedBy::Venue})
        QCOMPARE(int(ports::providedByFromKey(ports::toKey(p))), int(p));
    for (auto d : {PortDirection::Input, PortDirection::Output})
        QCOMPARE(int(ports::directionFromKey(ports::toKey(d))), int(d));
}

void TestPorts::displaysAreNonEmpty()
{
    QCOMPARE(ports::connectorDisplays().size(), 6);
    QCOMPARE(ports::levelDisplays().size(), 4);
    QCOMPARE(ports::signalDisplays().size(), 2);
    for (const QString &s : ports::connectorDisplays())
        QVERIFY(!s.isEmpty());
}

void TestPorts::portJsonRoundTrip()
{
    Port p;
    p.label = QStringLiteral("Out");
    p.direction = PortDirection::Output;
    p.connector = ConnectorType::QuarterTrs;
    p.level = SignalLevel::Line;
    p.signal = SignalConfig::Stereo;
    p.balanced = false;
    p.phantom = true;
    p.providedBy = ProvidedBy::Venue;
    p.toConsole = true;
    p.notes = QStringLiteral("keys DI");

    const Port q = Port::fromJson(p.toJson());
    QCOMPARE(q.label, p.label);
    QCOMPARE(int(q.direction), int(p.direction));
    QCOMPARE(int(q.connector), int(p.connector));
    QCOMPARE(int(q.level), int(p.level));
    QCOMPARE(int(q.signal), int(p.signal));
    QCOMPARE(q.balanced, p.balanced);
    QCOMPARE(q.phantom, p.phantom);
    QCOMPARE(int(q.providedBy), int(p.providedBy));
    QCOMPARE(q.toConsole, p.toConsole);
    QCOMPARE(q.notes, p.notes);

    QCOMPARE(q.channelCount(), 2);        // stereo
    QVERIFY(q.isConsoleChannel());        // output + toConsole
}

QTEST_APPLESS_MAIN(TestPorts)
#include "test_ports.moc"