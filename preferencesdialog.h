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

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>

class QComboBox;

// Application preferences. Page setup is kept per operating feature (today only
// the stage plot) so future features — e.g. a set list — can default to their
// own size/orientation. On accept, the chosen values are written to QSettings;
// callers re-read them via pageconfig::load().
class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

private:
    void save();

    QComboBox *m_stagePlotSize = nullptr;
    QComboBox *m_stagePlotOrientation = nullptr;
};

#endif // PREFERENCESDIALOG_H