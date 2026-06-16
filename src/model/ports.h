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

#ifndef PORTS_H
#define PORTS_H

#include <QString>
#include <QStringList>

class QJsonObject;

enum class PortDirection { Input, Output };
enum class ConnectorType { Xlr, QuarterTs, QuarterTrs, Speakon, Rca, Other };
enum class SignalLevel { Mic, Instrument, Line, Speaker };
enum class SignalConfig { Mono, Stereo };
enum class ProvidedBy { Unspecified, Band, Venue };

// One connection on a device. Used both as a catalog template and as the
// per-instance configuration on a placed device.
struct Port
{
    QString label;
    PortDirection direction = PortDirection::Output;
    ConnectorType connector = ConnectorType::Xlr;
    SignalLevel level = SignalLevel::Mic;
    SignalConfig signal = SignalConfig::Mono;
    bool balanced = true;
    bool phantom = false;
    ProvidedBy providedBy = ProvidedBy::Unspecified;
    bool toConsole = true;
    QString notes;

    // An output flagged toConsole becomes one or more numbered channels.
    bool isConsoleChannel() const
    {
        return toConsole && direction == PortDirection::Output;
    }
    // Stereo occupies two channel numbers (L/R); everything else one.
    int channelCount() const { return signal == SignalConfig::Stereo ? 2 : 1; }

    QJsonObject toJson() const;
    static Port fromJson(const QJsonObject &obj);
};

// Enum <-> JSON-key and display-string helpers. Display lists are ordered to
// match the enum's integer values, so a combo box index maps straight to the
// enum value.
namespace ports {

QString toKey(PortDirection d);
QString toKey(ConnectorType c);
QString toKey(SignalLevel l);
QString toKey(SignalConfig s);
QString toKey(ProvidedBy p);

PortDirection directionFromKey(const QString &key);
ConnectorType connectorFromKey(const QString &key);
SignalLevel levelFromKey(const QString &key);
SignalConfig signalFromKey(const QString &key);
ProvidedBy providedByFromKey(const QString &key);

QString display(PortDirection d);
QString display(ConnectorType c);
QString display(SignalLevel l);
QString display(SignalConfig s);
QString display(ProvidedBy p);

QStringList directionDisplays();
QStringList connectorDisplays();
QStringList levelDisplays();
QStringList signalDisplays();
QStringList providedByDisplays();

} // namespace ports

#endif // PORTS_H