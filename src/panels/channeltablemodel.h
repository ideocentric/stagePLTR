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

#ifndef CHANNELTABLEMODEL_H
#define CHANNELTABLEMODEL_H

#include "stagescene.h"  // Channel

#include <QAbstractTableModel>
#include <QList>

// Read-only table model presenting the breakout / input list. Rebuilt wholesale
// from the scene's channel list whenever it changes (channels are derived, so
// editing happens on the device, not here).
class ChannelTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColNumber = 0,
        ColSource,
        ColConnector,
        ColSignal,
        ColLevel,
        ColBalanced,
        ColPhantom,
        ColProvidedBy,
        ColNotes,
        ColumnCount
    };

    explicit ChannelTableModel(QObject *parent = nullptr);

    void setChannels(const QList<Channel> &channels);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

private:
    QList<Channel> m_channels;
};

#endif // CHANNELTABLEMODEL_H