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

#ifndef PAGECONFIG_H
#define PAGECONFIG_H

#include <QList>
#include <QPageLayout>
#include <QPageSize>
#include <QSizeF>
#include <QString>

class QJsonObject;

// An operating feature of the app that owns its own page setup. Today only the
// stage plot exists; future features (e.g. a set list) get their own entry so
// each can default to a different size/orientation independently.
enum class DocumentFeature {
    StagePlot,
};

// Page size + orientation for one feature. Pixel dimensions are derived at a
// fixed 96 dpi so scene units stay stable regardless of the export device.
struct PageConfig
{
    QPageSize::PageSizeId sizeId = QPageSize::Letter;
    QPageLayout::Orientation orientation = QPageLayout::Portrait;

    // Page dimensions in scene pixels (96 dpi), with orientation applied.
    QSizeF pixelSize() const;

    QJsonObject toJson() const;
    static PageConfig fromJson(const QJsonObject &obj);

    bool operator==(const PageConfig &o) const
    {
        return sizeId == o.sizeId && orientation == o.orientation;
    }
    bool operator!=(const PageConfig &o) const { return !(*this == o); }
};

namespace pageconfig {

// Per-feature factory default (used when nothing is stored yet).
PageConfig defaults(DocumentFeature feature);

// The user's global default for a feature, persisted via QSettings. New
// documents start from this; a saved .splot also carries its own copy.
PageConfig load(DocumentFeature feature);
void save(DocumentFeature feature, const PageConfig &config);

// Page sizes offered in the Preferences UI, and their display names.
QList<QPageSize::PageSizeId> sizeChoices();
QString sizeName(QPageSize::PageSizeId id);

} // namespace pageconfig

#endif // PAGECONFIG_H