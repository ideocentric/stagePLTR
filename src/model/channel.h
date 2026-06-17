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

#ifndef CHANNEL_H
#define CHANNEL_H

#include <QString>

// One row of the breakout / input list: a numbered console channel derived from
// a device's console-bound output port. Fields are pre-formatted for display.
// Produced by StageScene::renumberChannels() and consumed by the breakout
// table model.
struct Channel
{
    int number = 0;
    QString source;       // device label (+ " (L)" / " (R)" for stereo)
    QString connector;    // e.g. "XLR"
    QString signal;       // "Mono", "L", or "R"
    QString level;        // e.g. "Mic"
    bool balanced = true;
    bool phantom = false;
    QString providedBy;   // "Band" / "Venue" / "—"
    QString notes;
};

#endif // CHANNEL_H