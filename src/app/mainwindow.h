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
#include "ports.h"
#include "stagescene.h"  // StageScene + DeviceTransform (for slot signatures)

#include <QByteArray>
#include <QList>
#include <QMainWindow>
#include <QString>

class StageView;
class DevicePalette;
class PortEditor;
class ChannelTableModel;
class DeviceItem;
class QAction;
class QCloseEvent;
class QShowEvent;
class QTableView;
class QUndoStack;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void handleDrop(const QString &typeId, const QPointF &scenePos);
    void handleTransforms(const QVector<DeviceTransform> &changes);
    void handlePortEdit();
    void showObjectMenu(const QString &typeId, const QPoint &globalPos);
    void resetWindowLayout();
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

    // Window placement (persisted via QSettings; scaling-aware centering).
    void restoreWindowGeometry();
    void saveWindowGeometry();
    void centerOnScreen();
    QSize defaultWindowSize() const;

    // Object-library actions (right-click a palette item).
    void duplicateObject(const QString &id);
    void editObject(const QString &id);
    void deleteObject(const QString &id);
    void openObjectEditor(const DeviceType &seed);
    QString makeUniqueObjectId(const QString &base) const;

    bool maybeSave();                       // prompt if modified; false to cancel
    bool saveToFile(const QString &path);
    bool loadFromFile(const QString &path);
    void setCurrentFile(const QString &path);
    void refreshProperties();               // re-bind the editor + edit baseline

    DeviceCatalog m_catalog;
    StageScene *m_scene = nullptr;
    StageView *m_view = nullptr;
    DevicePalette *m_palette = nullptr;
    PortEditor *m_portEditor = nullptr;
    ChannelTableModel *m_channelModel = nullptr;
    QTableView *m_breakoutView = nullptr;
    QUndoStack *m_undoStack = nullptr;
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;

    QString m_currentFile;

    // Baseline for turning Properties-panel edits into one undo step each.
    DeviceItem *m_editDevice = nullptr;
    QString m_editLabel;
    QList<Port> m_editPorts;
    bool m_suppressRefresh = false;  // don't re-bind the editor during its own edit

    QByteArray m_defaultLayout;     // dock arrangement captured at startup
    bool m_pendingCenter = false;   // centre on the first show (first run only)
};

#endif // MAINWINDOW_H