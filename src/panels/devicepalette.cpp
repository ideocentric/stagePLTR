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
#include <QScrollBar>

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
    // Remember where the user was so a rebuild (after an import or a delete)
    // doesn't collapse their section and dump them back at the top — that makes
    // removing several objects in a row disorienting.
    const QString openCategory = currentCategory();
    int scroll = 0;
    int selectedRow = -1;
    if (auto *current = qobject_cast<DeviceList *>(currentWidget())) {
        scroll = current->verticalScrollBar()->value();
        selectedRow = current->currentRow();
    }

    clearSections();

    int restoreIndex = -1;
    for (const QString &category : catalog.orderedCategories()) {
        auto *list = new DeviceList;
        list->setProperty("category", category);
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
        const int index = addItem(list, label);
        if (category == openCategory)
            restoreIndex = index;
    }

    if (restoreIndex < 0)
        return;
    setCurrentIndex(restoreIndex);
    if (auto *list = qobject_cast<DeviceList *>(widget(restoreIndex))) {
        list->verticalScrollBar()->setValue(scroll);
        // Re-select a neighbour of the just-removed row so repeated right-click →
        // Delete keeps working without hunting for the next item.
        const int rows = list->count();
        if (selectedRow >= 0 && rows > 0)
            list->setCurrentRow(qMin(selectedRow, rows - 1));
    }
}

QString DevicePalette::currentCategory() const
{
    if (QWidget *page = currentWidget())
        return page->property("category").toString();
    return QString();
}