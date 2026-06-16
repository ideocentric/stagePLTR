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

#include <QJsonObject>

namespace ports {

QString toKey(PortDirection d)
{
    return d == PortDirection::Input ? QStringLiteral("input") : QStringLiteral("output");
}

QString toKey(ConnectorType c)
{
    switch (c) {
    case ConnectorType::Xlr: return QStringLiteral("xlr");
    case ConnectorType::QuarterTs: return QStringLiteral("ts");
    case ConnectorType::QuarterTrs: return QStringLiteral("trs");
    case ConnectorType::Speakon: return QStringLiteral("speakon");
    case ConnectorType::Rca: return QStringLiteral("rca");
    case ConnectorType::Other: return QStringLiteral("other");
    }
    return QStringLiteral("xlr");
}

QString toKey(SignalLevel l)
{
    switch (l) {
    case SignalLevel::Mic: return QStringLiteral("mic");
    case SignalLevel::Instrument: return QStringLiteral("instrument");
    case SignalLevel::Line: return QStringLiteral("line");
    case SignalLevel::Speaker: return QStringLiteral("speaker");
    }
    return QStringLiteral("line");
}

QString toKey(SignalConfig s)
{
    return s == SignalConfig::Stereo ? QStringLiteral("stereo") : QStringLiteral("mono");
}

QString toKey(ProvidedBy p)
{
    switch (p) {
    case ProvidedBy::Band: return QStringLiteral("band");
    case ProvidedBy::Venue: return QStringLiteral("venue");
    case ProvidedBy::Unspecified: return QStringLiteral("unspecified");
    }
    return QStringLiteral("unspecified");
}

PortDirection directionFromKey(const QString &key)
{
    return key == QLatin1String("input") ? PortDirection::Input : PortDirection::Output;
}

ConnectorType connectorFromKey(const QString &key)
{
    if (key == QLatin1String("xlr")) return ConnectorType::Xlr;
    if (key == QLatin1String("ts")) return ConnectorType::QuarterTs;
    if (key == QLatin1String("trs")) return ConnectorType::QuarterTrs;
    if (key == QLatin1String("speakon")) return ConnectorType::Speakon;
    if (key == QLatin1String("rca")) return ConnectorType::Rca;
    return ConnectorType::Other;
}

SignalLevel levelFromKey(const QString &key)
{
    if (key == QLatin1String("mic")) return SignalLevel::Mic;
    if (key == QLatin1String("instrument")) return SignalLevel::Instrument;
    if (key == QLatin1String("speaker")) return SignalLevel::Speaker;
    return SignalLevel::Line;
}

SignalConfig signalFromKey(const QString &key)
{
    return key == QLatin1String("stereo") ? SignalConfig::Stereo : SignalConfig::Mono;
}

ProvidedBy providedByFromKey(const QString &key)
{
    if (key == QLatin1String("band")) return ProvidedBy::Band;
    if (key == QLatin1String("venue")) return ProvidedBy::Venue;
    return ProvidedBy::Unspecified;
}

QString display(PortDirection d)
{
    return d == PortDirection::Input ? QStringLiteral("Input") : QStringLiteral("Output");
}

QString display(ConnectorType c)
{
    switch (c) {
    case ConnectorType::Xlr: return QStringLiteral("XLR");
    case ConnectorType::QuarterTs: return QString::fromUtf8("¼\" TS");
    case ConnectorType::QuarterTrs: return QString::fromUtf8("¼\" TRS");
    case ConnectorType::Speakon: return QStringLiteral("speakON");
    case ConnectorType::Rca: return QStringLiteral("RCA");
    case ConnectorType::Other: return QStringLiteral("Other");
    }
    return QStringLiteral("XLR");
}

QString display(SignalLevel l)
{
    switch (l) {
    case SignalLevel::Mic: return QStringLiteral("Mic");
    case SignalLevel::Instrument: return QStringLiteral("Instrument");
    case SignalLevel::Line: return QStringLiteral("Line");
    case SignalLevel::Speaker: return QStringLiteral("Speaker");
    }
    return QStringLiteral("Line");
}

QString display(SignalConfig s)
{
    return s == SignalConfig::Stereo ? QStringLiteral("Stereo") : QStringLiteral("Mono");
}

QString display(ProvidedBy p)
{
    switch (p) {
    case ProvidedBy::Band: return QStringLiteral("Band");
    case ProvidedBy::Venue: return QStringLiteral("Venue");
    case ProvidedBy::Unspecified: return QString::fromUtf8("—");
    }
    return QString::fromUtf8("—");
}

QStringList directionDisplays()
{
    return {display(PortDirection::Input), display(PortDirection::Output)};
}

QStringList connectorDisplays()
{
    return {display(ConnectorType::Xlr), display(ConnectorType::QuarterTs),
            display(ConnectorType::QuarterTrs), display(ConnectorType::Speakon),
            display(ConnectorType::Rca), display(ConnectorType::Other)};
}

QStringList levelDisplays()
{
    return {display(SignalLevel::Mic), display(SignalLevel::Instrument),
            display(SignalLevel::Line), display(SignalLevel::Speaker)};
}

QStringList signalDisplays()
{
    return {display(SignalConfig::Mono), display(SignalConfig::Stereo)};
}

QStringList providedByDisplays()
{
    return {display(ProvidedBy::Unspecified), display(ProvidedBy::Band),
            display(ProvidedBy::Venue)};
}

} // namespace ports

QJsonObject Port::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("label")] = label;
    obj[QStringLiteral("direction")] = ports::toKey(direction);
    obj[QStringLiteral("connector")] = ports::toKey(connector);
    obj[QStringLiteral("level")] = ports::toKey(level);
    obj[QStringLiteral("signal")] = ports::toKey(signal);
    obj[QStringLiteral("balanced")] = balanced;
    obj[QStringLiteral("phantom")] = phantom;
    obj[QStringLiteral("providedBy")] = ports::toKey(providedBy);
    obj[QStringLiteral("toConsole")] = toConsole;
    if (!notes.isEmpty())
        obj[QStringLiteral("notes")] = notes;
    return obj;
}

Port Port::fromJson(const QJsonObject &obj)
{
    Port p;
    p.label = obj.value(QStringLiteral("label")).toString();
    p.direction = ports::directionFromKey(obj.value(QStringLiteral("direction")).toString());
    p.connector = ports::connectorFromKey(obj.value(QStringLiteral("connector")).toString());
    p.level = ports::levelFromKey(obj.value(QStringLiteral("level")).toString());
    p.signal = ports::signalFromKey(obj.value(QStringLiteral("signal")).toString());
    p.balanced = obj.value(QStringLiteral("balanced")).toBool(true);
    p.phantom = obj.value(QStringLiteral("phantom")).toBool(false);
    p.providedBy = ports::providedByFromKey(obj.value(QStringLiteral("providedBy")).toString());
    p.toConsole = obj.value(QStringLiteral("toConsole")).toBool(true);
    p.notes = obj.value(QStringLiteral("notes")).toString();
    return p;
}