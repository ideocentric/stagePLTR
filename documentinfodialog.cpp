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

#include "documentinfodialog.h"

#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRectF>
#include <QSvgRenderer>
#include <QVBoxLayout>

DocumentInfoDialog::DocumentInfoDialog(const DocumentInfo &info, QWidget *parent)
    : QDialog(parent)
    , m_logoData(info.logoData)
    , m_logoFormat(info.logoFormat)
{
    setWindowTitle(tr("Document Info"));

    m_bandName = new QLineEdit(info.bandName);
    m_bandName->setPlaceholderText(tr("Band or artist name"));

    m_date = new QDateEdit;
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat(QStringLiteral("MMMM d, yyyy"));
    m_date->setDate(info.date.isValid() ? info.date : QDate::currentDate());

    m_logoPreview = new QLabel;
    m_logoPreview->setFixedSize(160, 64);
    m_logoPreview->setAlignment(Qt::AlignCenter);
    m_logoPreview->setFrameShape(QFrame::StyledPanel);

    auto *chooseButton = new QPushButton(tr("Choose…"));
    auto *clearButton = new QPushButton(tr("Clear"));
    connect(chooseButton, &QPushButton::clicked, this, &DocumentInfoDialog::chooseLogo);
    connect(clearButton, &QPushButton::clicked, this, &DocumentInfoDialog::clearLogo);

    auto *logoButtons = new QVBoxLayout;
    logoButtons->addWidget(chooseButton);
    logoButtons->addWidget(clearButton);
    logoButtons->addStretch();

    auto *logoRow = new QHBoxLayout;
    logoRow->addWidget(m_logoPreview);
    logoRow->addLayout(logoButtons);
    logoRow->addStretch();

    auto *form = new QFormLayout;
    form->addRow(tr("Band name:"), m_bandName);
    form->addRow(tr("Date:"), m_date);
    form->addRow(tr("Logo:"), logoRow);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    auto *hint = new QLabel(tr("When a logo is set, it replaces the band name in the title block."));
    hint->setEnabled(false);
    hint->setWordWrap(true);
    form->addRow(QString(), hint);

    updateLogoPreview();
}

DocumentInfo DocumentInfoDialog::info() const
{
    DocumentInfo info;
    info.bandName = m_bandName->text().trimmed();
    info.date = m_date->date();
    info.logoData = m_logoData;
    info.logoFormat = m_logoFormat;
    return info;
}

void DocumentInfoDialog::chooseLogo()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Choose Logo"), QString(),
        tr("Images (*.png *.jpg *.jpeg *.svg)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    m_logoData = file.readAll();
    m_logoFormat = QFileInfo(path).suffix().toLower();
    updateLogoPreview();
}

void DocumentInfoDialog::clearLogo()
{
    m_logoData.clear();
    m_logoFormat.clear();
    updateLogoPreview();
}

void DocumentInfoDialog::updateLogoPreview()
{
    if (m_logoData.isEmpty()) {
        m_logoPreview->setText(tr("(no logo)"));
        return;
    }

    const QSize box = m_logoPreview->size() - QSize(8, 8);
    QPixmap pix(box);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        if (m_logoFormat.compare(QLatin1String("svg"), Qt::CaseInsensitive) == 0) {
            QSvgRenderer renderer(m_logoData);
            QSizeF src = renderer.defaultSize();
            if (src.isEmpty())
                src = QSizeF(box);
            const qreal s = qMin(box.width() / src.width(), box.height() / src.height());
            const QSizeF dst = src * s;
            renderer.render(&p, QRectF((box.width() - dst.width()) / 2.0,
                                       (box.height() - dst.height()) / 2.0,
                                       dst.width(), dst.height()));
        } else {
            const QPixmap src = QPixmap::fromImage(QImage::fromData(m_logoData));
            if (!src.isNull()) {
                const QPixmap scaled =
                    src.scaled(box, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                p.drawPixmap((box.width() - scaled.width()) / 2,
                             (box.height() - scaled.height()) / 2, scaled);
            }
        }
    }
    m_logoPreview->setPixmap(pix);
}