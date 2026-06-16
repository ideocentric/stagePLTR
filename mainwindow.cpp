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

#include "mainwindow.h"

#include "channeltablemodel.h"
#include "devicepalette.h"
#include "deviceitem.h"
#include "documentinfodialog.h"
#include "pdfexporter.h"
#include "porteditor.h"
#include "stagescene.h"
#include "stageview.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDate>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QHeaderView>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTableView>

namespace {
const QString kFileFilter = QStringLiteral("stagePLTR Plot (*.splot)");
const QString kFileSuffix = QStringLiteral(".splot");
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QString error;
    if (!m_catalog.loadFromResource(QStringLiteral(":/plot/catalog.json"),
                                    QStringLiteral(":/plot"), &error)) {
        QMessageBox::warning(this, tr("Device Catalog"),
                             tr("Could not load the device catalog:\n%1").arg(error));
    }

    m_docInfo.date = QDate::currentDate();

    m_scene = new StageScene(&m_catalog, this);
    m_scene->setDocumentInfo(m_docInfo);
    m_view = new StageView(this);
    m_view->setScene(m_scene);
    setCentralWidget(m_view);

    createActions();
    createMenus();
    createPalette();
    createPropertiesDock();
    createBreakoutDock();

    connect(m_scene, &StageScene::plotChanged, this, &MainWindow::markDirty);
    connect(m_scene, &StageScene::selectionChanged, this, &MainWindow::updateSelection);
    connect(m_scene, &StageScene::channelsChanged, this, [this] {
        m_channelModel->setChannels(m_scene->channels());
    });

    statusBar()->showMessage(tr("Ready"));
    resize(1280, 900);
    setCurrentFile(QString());
}

MainWindow::~MainWindow()
{
    // The scene is a child QObject, so it is destroyed by ~QWidget's
    // deleteChildren() — after the MainWindow-derived part of this object is
    // already gone. Tearing the scene down deletes its DeviceItems, and
    // removing a selected item emits selectionChanged (and plotChanged), which
    // are wired to MainWindow slots. Delivering a signal to a half-destroyed
    // MainWindow makes Qt abort (assertObjectType). Sever those connections
    // now, while `this` is still a valid MainWindow.
    if (m_scene)
        m_scene->disconnect(this);
}

void MainWindow::createActions()
{
    // File actions use native standard shortcuts (⌘N/Ctrl+N, etc.).
    auto *newAct = new QAction(tr("&New"), this);
    newAct->setShortcut(QKeySequence::New);
    connect(newAct, &QAction::triggered, this, &MainWindow::newPlot);
    addAction(newAct);

    auto *openAct = new QAction(tr("&Open…"), this);
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::openPlot);
    addAction(openAct);

    auto *saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcut(QKeySequence::Save);
    connect(saveAct, &QAction::triggered, this, &MainWindow::savePlot);
    addAction(saveAct);

    auto *saveAsAct = new QAction(tr("Save &As…"), this);
    saveAsAct->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::savePlotAs);
    addAction(saveAsAct);

    auto *docInfoAct = new QAction(tr("Document &Info…"), this);
    docInfoAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    connect(docInfoAct, &QAction::triggered, this, &MainWindow::editDocumentInfo);
    addAction(docInfoAct);

    auto *exportPdfAct = new QAction(tr("&Export to PDF…"), this);
    exportPdfAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));
    connect(exportPdfAct, &QAction::triggered, this, &MainWindow::exportPdf);
    addAction(exportPdfAct);

    auto *quitAct = new QAction(tr("&Quit"), this);
    quitAct->setShortcut(QKeySequence::Quit);
    quitAct->setMenuRole(QAction::QuitRole);  // moves to app menu on macOS
    connect(quitAct, &QAction::triggered, this, &QWidget::close);
    addAction(quitAct);

    auto *deleteAct = new QAction(tr("&Delete"), this);
    deleteAct->setShortcuts({QKeySequence::Delete, QKeySequence(Qt::Key_Backspace)});
    connect(deleteAct, &QAction::triggered, this, &MainWindow::deleteSelection);
    addAction(deleteAct);

    auto *selectAllAct = new QAction(tr("Select &All"), this);
    selectAllAct->setShortcut(QKeySequence::SelectAll);
    connect(selectAllAct, &QAction::triggered, this, [this] {
        const QList<QGraphicsItem *> all = m_scene->items();
        for (QGraphicsItem *item : all)
            if (item->type() == DeviceItem::Type)
                item->setSelected(true);
    });
    addAction(selectAllAct);

    auto *rotateLeftAct = new QAction(tr("Rotate &Left"), this);
    rotateLeftAct->setShortcut(QKeySequence(Qt::Key_BracketLeft));
    connect(rotateLeftAct, &QAction::triggered, this, [this] { rotateSelection(-15.0); });
    addAction(rotateLeftAct);

    auto *rotateRightAct = new QAction(tr("Rotate &Right"), this);
    rotateRightAct->setShortcut(QKeySequence(Qt::Key_BracketRight));
    connect(rotateRightAct, &QAction::triggered, this, [this] { rotateSelection(15.0); });
    addAction(rotateRightAct);

    auto *zoomInAct = new QAction(tr("Zoom &In"), this);
    zoomInAct->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAct, &QAction::triggered, this, [this] { m_view->zoomIn(); });
    addAction(zoomInAct);

    auto *zoomOutAct = new QAction(tr("Zoom &Out"), this);
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAct, &QAction::triggered, this, [this] { m_view->zoomOut(); });
    addAction(zoomOutAct);

    auto *resetZoomAct = new QAction(tr("&Actual Size"), this);
    resetZoomAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    connect(resetZoomAct, &QAction::triggered, this, [this] { m_view->resetZoom(); });
    addAction(resetZoomAct);

    auto *aboutAct = new QAction(tr("&About stagePLTR"), this);
    aboutAct->setMenuRole(QAction::AboutRole);  // moves to app menu on macOS
    connect(aboutAct, &QAction::triggered, this, &MainWindow::about);
    addAction(aboutAct);

    // Keep references on the object for menu construction.
    m_view->addActions({newAct, openAct, saveAct, saveAsAct});
}

void MainWindow::createMenus()
{
    // Rebuild from the actions added in createActions() by querying them back.
    const QList<QAction *> all = actions();
    auto byText = [&all](const QString &text) -> QAction * {
        for (QAction *a : all)
            if (a->text() == text)
                return a;
        return nullptr;
    };

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(byText(tr("&New")));
    fileMenu->addAction(byText(tr("&Open…")));
    fileMenu->addSeparator();
    fileMenu->addAction(byText(tr("&Save")));
    fileMenu->addAction(byText(tr("Save &As…")));
    fileMenu->addSeparator();
    fileMenu->addAction(byText(tr("Document &Info…")));
    fileMenu->addAction(byText(tr("&Export to PDF…")));
    fileMenu->addSeparator();
    fileMenu->addAction(byText(tr("&Quit")));

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(byText(tr("&Delete")));
    editMenu->addAction(byText(tr("Select &All")));
    editMenu->addSeparator();
    editMenu->addAction(byText(tr("Rotate &Left")));
    editMenu->addAction(byText(tr("Rotate &Right")));

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(byText(tr("Zoom &In")));
    viewMenu->addAction(byText(tr("Zoom &Out")));
    viewMenu->addAction(byText(tr("&Actual Size")));

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(byText(tr("&About stagePLTR")));
}

void MainWindow::createPalette()
{
    auto *dock = new QDockWidget(tr("Devices"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_palette = new DevicePalette(dock);
    m_palette->populate(m_catalog);
    dock->setWidget(m_palette);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    connect(m_palette, &DevicePalette::deviceActivated,
            this, &MainWindow::addDeviceAtCentre);
}

void MainWindow::createPropertiesDock()
{
    auto *dock = new QDockWidget(tr("Properties"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_portEditor = new PortEditor(dock);
    dock->setWidget(m_portEditor);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    connect(m_portEditor, &PortEditor::edited, this, [this] {
        m_scene->renumberChannels();
        markDirty();
    });
}

void MainWindow::createBreakoutDock()
{
    auto *dock = new QDockWidget(tr("Breakout / Input List"), this);
    dock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    m_channelModel = new ChannelTableModel(this);
    m_breakoutView = new QTableView(dock);
    m_breakoutView->setModel(m_channelModel);
    m_breakoutView->verticalHeader()->setVisible(false);
    m_breakoutView->horizontalHeader()->setStretchLastSection(true);
    m_breakoutView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_breakoutView->setSelectionBehavior(QAbstractItemView::SelectRows);
    dock->setWidget(m_breakoutView);
    addDockWidget(Qt::BottomDockWidgetArea, dock);
}

void MainWindow::updateSelection()
{
    const QList<QGraphicsItem *> selected = m_scene->selectedItems();
    DeviceItem *device = nullptr;
    int deviceCount = 0;
    for (QGraphicsItem *item : selected) {
        if (item->type() == DeviceItem::Type) {
            device = static_cast<DeviceItem *>(item);
            ++deviceCount;
        }
    }
    // Show the editor only when exactly one device is selected.
    m_portEditor->setDevice(deviceCount == 1 ? device : nullptr);
}

void MainWindow::newPlot()
{
    if (!maybeSave())
        return;
    m_scene->clearDevices();
    m_docInfo = DocumentInfo();
    m_docInfo.date = QDate::currentDate();
    m_scene->setDocumentInfo(m_docInfo);
    setCurrentFile(QString());
    m_dirty = false;
    updateWindowTitle();
}

void MainWindow::openPlot()
{
    if (!maybeSave())
        return;
    const QString path =
        QFileDialog::getOpenFileName(this, tr("Open Plot"), QString(), kFileFilter);
    if (path.isEmpty())
        return;
    loadFromFile(path);
}

bool MainWindow::savePlot()
{
    if (m_currentFile.isEmpty())
        return savePlotAs();
    return saveToFile(m_currentFile);
}

bool MainWindow::savePlotAs()
{
    QString path =
        QFileDialog::getSaveFileName(this, tr("Save Plot As"), QString(), kFileFilter);
    if (path.isEmpty())
        return false;
    if (!path.endsWith(kFileSuffix, Qt::CaseInsensitive))
        path += kFileSuffix;
    return saveToFile(path);
}

void MainWindow::editDocumentInfo()
{
    DocumentInfoDialog dialog(m_docInfo, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_docInfo = dialog.info();
    m_scene->setDocumentInfo(m_docInfo);
    markDirty();
}

void MainWindow::exportPdf()
{
    const QString suggested =
        m_currentFile.isEmpty()
            ? QStringLiteral("stage-plot.pdf")
            : QFileInfo(m_currentFile).completeBaseName() + QStringLiteral(".pdf");

    QString path = QFileDialog::getSaveFileName(this, tr("Export to PDF"), suggested,
                                                tr("PDF (*.pdf)"));
    if (path.isEmpty())
        return;
    if (!path.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
        path += QStringLiteral(".pdf");

    const QString title =
        m_currentFile.isEmpty() ? tr("Untitled") : QFileInfo(m_currentFile).completeBaseName();

    QString error;
    if (PdfExporter::exportPlot(path, m_scene, title, &error))
        statusBar()->showMessage(tr("Exported %1").arg(QFileInfo(path).fileName()), 3000);
    else
        QMessageBox::warning(this, tr("Export Failed"), error);
}

void MainWindow::deleteSelection()
{
    m_scene->removeDevices(m_scene->selectedItems());
}

void MainWindow::rotateSelection(double degrees)
{
    const QList<QGraphicsItem *> selected = m_scene->selectedItems();
    for (QGraphicsItem *item : selected) {
        if (item->type() == DeviceItem::Type)
            item->setRotation(item->rotation() + degrees);
    }
}

void MainWindow::addDeviceAtCentre(const QString &typeId)
{
    const QPointF centre = m_view->mapToScene(m_view->viewport()->rect().center());
    m_scene->addDevice(typeId, centre);
}

void MainWindow::about()
{
    QMessageBox::about(
        this, tr("About stagePLTR"),
        tr("<h3>stagePLTR</h3>"
           "<p>An open-source tool for designing stage plots and tech riders for bands.</p>"
           "<p>Licensed under GPL-3.0-or-later.</p>"));
}

void MainWindow::updateWindowTitle()
{
    const QString name =
        m_currentFile.isEmpty() ? tr("Untitled") : QFileInfo(m_currentFile).fileName();
    setWindowTitle(tr("%1[*] — stagePLTR").arg(name));
    setWindowModified(m_dirty);
}

bool MainWindow::maybeSave()
{
    if (!m_dirty)
        return true;
    const auto choice = QMessageBox::warning(
        this, tr("stagePLTR"),
        tr("The plot has unsaved changes.\nDo you want to save them?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    switch (choice) {
    case QMessageBox::Save:
        return savePlot();
    case QMessageBox::Cancel:
        return false;
    default:
        return true;
    }
}

bool MainWindow::saveToFile(const QString &path)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Save Failed"),
                             tr("Could not write to %1").arg(path));
        return false;
    }
    QJsonObject root = m_scene->toJson();
    root[QStringLiteral("info")] = m_docInfo.toJson();
    const QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        QMessageBox::warning(this, tr("Save Failed"),
                             tr("Could not save %1").arg(path));
        return false;
    }
    setCurrentFile(path);
    m_dirty = false;
    updateWindowTitle();
    statusBar()->showMessage(tr("Saved %1").arg(QFileInfo(path).fileName()), 3000);
    return true;
}

bool MainWindow::loadFromFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Open Failed"),
                             tr("Could not open %1").arg(path));
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::warning(this, tr("Open Failed"),
                             tr("%1 is not a valid plot file:\n%2")
                                 .arg(QFileInfo(path).fileName(), parseError.errorString()));
        return false;
    }

    // Suppress dirty-marking while we populate the scene programmatically.
    {
        const QSignalBlocker blocker(m_scene);
        const QJsonObject root = doc.object();
        m_docInfo = DocumentInfo::fromJson(root.value(QStringLiteral("info")).toObject());
        m_scene->setDocumentInfo(m_docInfo);
        QString error;
        m_scene->fromJson(root, &error);
        if (!error.isEmpty())
            statusBar()->showMessage(error, 5000);
    }
    // Channel signals were blocked during load; sync the breakout view now.
    m_channelModel->setChannels(m_scene->channels());
    m_portEditor->setDevice(nullptr);

    setCurrentFile(path);
    m_dirty = false;
    updateWindowTitle();
    statusBar()->showMessage(tr("Opened %1").arg(QFileInfo(path).fileName()), 3000);
    return true;
}

void MainWindow::setCurrentFile(const QString &path)
{
    m_currentFile = path;
    updateWindowTitle();
}

void MainWindow::markDirty()
{
    if (!m_dirty) {
        m_dirty = true;
        updateWindowTitle();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}