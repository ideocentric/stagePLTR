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
#include "devicelist.h"

#include <QFont>

DevicePalette::DevicePalette(QWidget *parent)
    : QToolBox(parent)
{
    // A styled QToolBox::tab sizes its height from the plain font but renders the
    // selected tab bold (taller), which clips descenders. Fix the font size and
    // give the tab an explicit min-height so the label always fits.
    QFont tabFont = font();
    tabFont.setPixelSize(12);
    setFont(tabFont);

    // Keep the whole palette light regardless of OS theme so the dark device
    // icons stay legible, and so the section headers match the icon grids.
    setStyleSheet(QStringLiteral(
        "QToolBox { background-color: #e9e9e9; }"
        "QToolBox::tab {"
        "  background-color: #d8dadd;"
        "  color: #202020;"
        "  border: 1px solid #c2c4c7;"
        "  border-radius: 3px;"
        "  padding: 4px 8px;"
        "  min-height: 22px;"
        "}"
        "QToolBox::tab:selected { background-color: #cfe0f5; color: #101010;"
        "  font-weight: bold; }"
        "QToolBox::tab:hover { background-color: #e2eefb; }"));
}

void DevicePalette::clearSections()
{
    while (count() > 0) {
        QWidget *page = widget(0);
        removeItem(0);
        delete page;
    }
}

void DevicePalette::populate(const DeviceCatalog &catalog)
{
    clearSections();

    for (const QString &category : catalog.orderedCategories()) {
        auto *list = new DeviceList;
        int added = 0;
        for (const DeviceType &type : catalog.devices()) {
            if (type.category != category)
                continue;
            list->addDevice(type);
            ++added;
        }
        if (added == 0)
            list->addPlaceholder(tr("Coming soon"));

        connect(list, &DeviceList::deviceActivated, this, &DevicePalette::deviceActivated);
        connect(list, &DeviceList::contextMenuRequested, this,
                &DevicePalette::objectContextMenu);
        const QString label = added > 0 ? QStringLiteral("%1  (%2)").arg(category).arg(added)
                                        : category;
        addItem(list, label);
    }
}