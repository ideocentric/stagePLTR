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

#include "channeltablemodel.h"

ChannelTableModel::ChannelTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void ChannelTableModel::setChannels(const QList<Channel> &channels)
{
    beginResetModel();
    m_channels = channels;
    endResetModel();
}

int ChannelTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_channels.size();
}

int ChannelTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ChannelTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_channels.size())
        return {};

    const Channel &ch = m_channels.at(index.row());

    if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
        case ColNumber:
        case ColBalanced:
        case ColPhantom:
            return int(Qt::AlignCenter);
        default:
            return int(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }

    if (role != Qt::DisplayRole)
        return {};

    switch (index.column()) {
    case ColNumber: return ch.number;
    case ColSource: return ch.source;
    case ColConnector: return ch.connector;
    case ColSignal: return ch.signal;
    case ColLevel: return ch.level;
    case ColBalanced: return ch.balanced ? QStringLiteral("Bal") : QStringLiteral("Unbal");
    case ColPhantom: return ch.phantom ? QStringLiteral("+48V") : QString();
    case ColProvidedBy: return ch.providedBy;
    case ColNotes: return ch.notes;
    default: return {};
    }
}

QVariant ChannelTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};
    if (orientation == Qt::Vertical)
        return {};

    switch (section) {
    case ColNumber: return QStringLiteral("Ch");
    case ColSource: return QStringLiteral("Source");
    case ColConnector: return QStringLiteral("Connector");
    case ColSignal: return QStringLiteral("Signal");
    case ColLevel: return QStringLiteral("Level");
    case ColBalanced: return QStringLiteral("Bal.");
    case ColPhantom: return QStringLiteral("+48V");
    case ColProvidedBy: return QStringLiteral("Provided By");
    case ColNotes: return QStringLiteral("Notes");
    default: return {};
    }
}