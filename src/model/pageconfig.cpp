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

#include "pageconfig.h"

#include <QJsonObject>
#include <QSettings>

namespace {

constexpr qreal kScreenDpi = 96.0;  // scene units are pixels at this density

// Stable string keys for serialization (QSettings + .splot JSON). We only
// support a curated subset, so map explicitly rather than via QPageSize::key().
QString sizeKey(QPageSize::PageSizeId id)
{
    switch (id) {
    case QPageSize::A4:
        return QStringLiteral("A4");
    case QPageSize::Letter:
    default:
        return QStringLiteral("Letter");
    }
}

QPageSize::PageSizeId sizeFromKey(const QString &key)
{
    if (key.compare(QLatin1String("A4"), Qt::CaseInsensitive) == 0)
        return QPageSize::A4;
    return QPageSize::Letter;
}

QString orientationKey(QPageLayout::Orientation o)
{
    return o == QPageLayout::Landscape ? QStringLiteral("landscape")
                                       : QStringLiteral("portrait");
}

QPageLayout::Orientation orientationFromKey(const QString &key)
{
    return key.compare(QLatin1String("landscape"), Qt::CaseInsensitive) == 0
               ? QPageLayout::Landscape
               : QPageLayout::Portrait;
}

QString featureKey(DocumentFeature feature)
{
    switch (feature) {
    case DocumentFeature::StagePlot:
    default:
        return QStringLiteral("stagePlot");
    }
}

} // namespace

QSizeF PageConfig::pixelSize() const
{
    // QPageSize reports standard sizes in portrait (width <= height). Convert
    // points (1/72") to screen pixels, then apply orientation.
    const QSizeF pts = QPageSize(sizeId).size(QPageSize::Point);
    QSizeF px(pts.width() * kScreenDpi / 72.0, pts.height() * kScreenDpi / 72.0);
    if (orientation == QPageLayout::Landscape)
        px.transpose();
    return px;
}

QJsonObject PageConfig::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("size")] = sizeKey(sizeId);
    obj[QStringLiteral("orientation")] = orientationKey(orientation);
    return obj;
}

PageConfig PageConfig::fromJson(const QJsonObject &obj)
{
    PageConfig config;
    config.sizeId = sizeFromKey(obj.value(QStringLiteral("size")).toString());
    config.orientation =
        orientationFromKey(obj.value(QStringLiteral("orientation")).toString());
    return config;
}

namespace pageconfig {

PageConfig defaults(DocumentFeature feature)
{
    switch (feature) {
    case DocumentFeature::StagePlot:
    default:
        // Portrait Letter; a large rig can switch to Landscape in Preferences.
        return PageConfig{QPageSize::Letter, QPageLayout::Portrait};
    }
}

PageConfig load(DocumentFeature feature)
{
    const PageConfig def = defaults(feature);
    QSettings settings;
    settings.beginGroup(QStringLiteral("page/") + featureKey(feature));
    PageConfig config;
    config.sizeId = sizeFromKey(
        settings.value(QStringLiteral("size"), sizeKey(def.sizeId)).toString());
    config.orientation = orientationFromKey(
        settings.value(QStringLiteral("orientation"), orientationKey(def.orientation))
            .toString());
    settings.endGroup();
    return config;
}

void save(DocumentFeature feature, const PageConfig &config)
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("page/") + featureKey(feature));
    settings.setValue(QStringLiteral("size"), sizeKey(config.sizeId));
    settings.setValue(QStringLiteral("orientation"), orientationKey(config.orientation));
    settings.endGroup();
}

QList<QPageSize::PageSizeId> sizeChoices()
{
    return {QPageSize::Letter, QPageSize::A4};
}

QString sizeName(QPageSize::PageSizeId id)
{
    switch (id) {
    case QPageSize::A4:
        return QStringLiteral("A4");
    case QPageSize::Letter:
    default:
        return QStringLiteral("US Letter");
    }
}

} // namespace pageconfig