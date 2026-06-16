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

#ifndef DOCUMENTINFODIALOG_H
#define DOCUMENTINFODIALOG_H

#include "documentinfo.h"

#include <QDialog>

class QLineEdit;
class QDateEdit;
class QLabel;

// Edits a DocumentInfo: band name, date, and an embedded logo (the logo
// replaces the band name in the title block when set).
class DocumentInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DocumentInfoDialog(const DocumentInfo &info, QWidget *parent = nullptr);

    DocumentInfo info() const;

private:
    void chooseLogo();
    void clearLogo();
    void updateLogoPreview();

    QLineEdit *m_bandName = nullptr;
    QDateEdit *m_date = nullptr;
    QLabel *m_logoPreview = nullptr;

    QByteArray m_logoData;
    QString m_logoFormat;
};

#endif // DOCUMENTINFODIALOG_H