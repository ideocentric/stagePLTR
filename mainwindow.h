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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "devicecatalog.h"
#include "documentinfo.h"

#include <QMainWindow>
#include <QString>

class StageScene;
class StageView;
class DevicePalette;
class PortEditor;
class ChannelTableModel;
class QCloseEvent;
class QTableView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newPlot();
    void openPlot();
    bool savePlot();
    bool savePlotAs();
    void exportPdf();
    void editDocumentInfo();
    void openPreferences();
    void deleteSelection();
    void rotateSelection(double degrees);
    void addDeviceAtCentre(const QString &typeId);
    void about();
    void updateWindowTitle();

private:
    void createActions();
    void createMenus();
    void createPalette();
    void createPropertiesDock();
    void createBreakoutDock();
    void updateSelection();

    bool maybeSave();                       // prompt if dirty; returns false to cancel
    bool saveToFile(const QString &path);
    bool loadFromFile(const QString &path);
    void setCurrentFile(const QString &path);
    void markDirty();

    DeviceCatalog m_catalog;
    StageScene *m_scene = nullptr;
    StageView *m_view = nullptr;
    DevicePalette *m_palette = nullptr;
    PortEditor *m_portEditor = nullptr;
    ChannelTableModel *m_channelModel = nullptr;
    QTableView *m_breakoutView = nullptr;

    DocumentInfo m_docInfo;
    QString m_currentFile;
    bool m_dirty = false;
};

#endif // MAINWINDOW_H