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

#include "preferencesdialog.h"

#include "pageconfig.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QVBoxLayout>

namespace {

// Add a size + orientation row pair to `form`, pre-selecting `config`.
void buildPageSetup(QFormLayout *form, QComboBox *size, QComboBox *orientation,
                    const PageConfig &config)
{
    for (QPageSize::PageSizeId id : pageconfig::sizeChoices())
        size->addItem(pageconfig::sizeName(id), int(id));
    size->setCurrentIndex(qMax(0, size->findData(int(config.sizeId))));

    orientation->addItem(QObject::tr("Portrait"), int(QPageLayout::Portrait));
    orientation->addItem(QObject::tr("Landscape"), int(QPageLayout::Landscape));
    orientation->setCurrentIndex(
        qMax(0, orientation->findData(int(config.orientation))));

    form->addRow(QObject::tr("Page size:"), size);
    form->addRow(QObject::tr("Orientation:"), orientation);
}

PageConfig readPageSetup(const QComboBox *size, const QComboBox *orientation)
{
    PageConfig config;
    config.sizeId = QPageSize::PageSizeId(size->currentData().toInt());
    config.orientation = QPageLayout::Orientation(orientation->currentData().toInt());
    return config;
}

} // namespace

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
    , m_stagePlotSize(new QComboBox)
    , m_stagePlotOrientation(new QComboBox)
{
    setWindowTitle(tr("Preferences"));

    // Page setup is grouped per feature; add a group box here as features grow.
    auto *stagePlotBox = new QGroupBox(tr("Stage Plot — Page Setup"));
    auto *stagePlotForm = new QFormLayout(stagePlotBox);
    buildPageSetup(stagePlotForm, m_stagePlotSize, m_stagePlotOrientation,
                   pageconfig::load(DocumentFeature::StagePlot));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        save();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(stagePlotBox);
    layout->addStretch();
    layout->addWidget(buttons);
}

void PreferencesDialog::save()
{
    pageconfig::save(DocumentFeature::StagePlot,
                     readPageSetup(m_stagePlotSize, m_stagePlotOrientation));
}