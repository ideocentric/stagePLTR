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

#include "devicepalette.h"

#include "devicecatalog.h"
#include "stagescene.h"  // for kDeviceMimeType

#include <QIcon>
#include <QMimeData>

namespace {
constexpr int kIdRole = Qt::UserRole + 1;
}

DevicePalette::DevicePalette(QWidget *parent)
    : QListWidget(parent)
{
    setViewMode(QListView::IconMode);
    setIconSize(QSize(48, 48));
    setGridSize(QSize(84, 78));
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    setSpacing(4);
    setWordWrap(true);
    setUniformItemSizes(true);
    setDragEnabled(true);
    setSelectionMode(QAbstractItemView::SingleSelection);

    // The device icons are dark line-art, so in OS dark mode they'd be dark on a
    // dark panel and disappear. Pin the palette to a light-gray background with
    // dark text via a style sheet (overrides the native dark theme on every
    // platform, unlike a QPalette override which the macOS style can ignore).
    setStyleSheet(QStringLiteral(
        "QListWidget {"
        "  background-color: #e9e9e9;"
        "  color: #202020;"
        "  border: none;"
        "}"
        "QListWidget::item {"
        "  color: #202020;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #bcd4f6;"
        "  color: #101010;"
        "}"));

    connect(this, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        if (item)
            emit deviceActivated(idOf(item));
    });
}

void DevicePalette::populate(const DeviceCatalog &catalog)
{
    clear();
    for (const QString &category : catalog.categories()) {
        for (const DeviceType &type : catalog.devices()) {
            if (type.category != category)
                continue;
            auto *item = new QListWidgetItem(QIcon(type.iconResource), type.name, this);
            item->setData(kIdRole, type.id);
            item->setToolTip(QStringLiteral("%1 — %2").arg(type.name, type.category));
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
        }
    }
}

QString DevicePalette::idOf(const QListWidgetItem *item)
{
    return item ? item->data(kIdRole).toString() : QString();
}

QStringList DevicePalette::mimeTypes() const
{
    return {QString::fromLatin1(kDeviceMimeType)};
}

QMimeData *DevicePalette::mimeData(const QList<QListWidgetItem *> &items) const
{
    if (items.isEmpty())
        return nullptr;
    const QString id = idOf(items.first());
    if (id.isEmpty())
        return nullptr;

    auto *mime = new QMimeData;
    mime->setData(QString::fromLatin1(kDeviceMimeType), id.toUtf8());
    return mime;
}