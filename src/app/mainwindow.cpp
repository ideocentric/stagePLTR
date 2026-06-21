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
#include "objecteditordialog.h"
#include "pageconfig.h"
#include "pdfexporter.h"
#include "porteditor.h"
#include "preferencesdialog.h"
#include "stagescene.h"
#include "stageview.h"
#include "undocommands.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCursor>
#include <QDate>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHash>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QHeaderView>
#include <QSaveFile>
#include <QScreen>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QLineEdit>
#include <QStatusBar>
#include <QTableView>
#include <QUndoStack>
#include <QUuid>

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
    // Merge the user's own objects (created via duplicate/edit) from app data.
    m_catalog.setUserLibraryPath(
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
            .filePath(QStringLiteral("objects")));
    m_catalog.loadUserLibrary();

    m_scene = new StageScene(&m_catalog, this);
    m_scene->setPageConfig(pageconfig::load(DocumentFeature::StagePlot));
    {
        DocumentInfo info;
        info.date = QDate::currentDate();
        m_scene->setDocumentInfo(info);
    }
    m_undoStack = new QUndoStack(this);

    m_view = new StageView(this);
    m_view->setScene(m_scene);
    setCentralWidget(m_view);

    createActions();
    createMenus();
    createPalette();
    createPropertiesDock();
    createBreakoutDock();

    connect(m_scene, &StageScene::selectionChanged, this, &MainWindow::refreshProperties);
    connect(m_scene, &StageScene::channelsChanged, this, [this] {
        m_channelModel->setChannels(m_scene->channels());
    });
    connect(m_scene, &StageScene::editDocumentInfoRequested,
            this, &MainWindow::editDocumentInfo);
    connect(m_scene, &StageScene::dropRequested, this, &MainWindow::handleDrop);
    connect(m_scene, &StageScene::devicesTransformed, this, &MainWindow::handleTransforms);

    // The undo stack's clean state drives the document-modified indicator; its
    // index changes re-sync the Properties panel after undo/redo.
    connect(m_undoStack, &QUndoStack::cleanChanged, this, [this] { updateWindowTitle(); });
    connect(m_undoStack, &QUndoStack::indexChanged, this, [this] {
        if (!m_suppressRefresh)
            refreshProperties();
    });

    statusBar()->showMessage(tr("Ready"));

    // Remember the default dock arrangement (for "Reset Window Layout"), then
    // restore the saved window geometry/state or fall back to a centred default.
    setObjectName(QStringLiteral("MainWindow"));
    m_defaultLayout = saveState();
    restoreWindowGeometry();

    setCurrentFile(QString());
}

MainWindow::~MainWindow()
{
    // The scene and the undo stack are child QObjects, destroyed by ~QWidget's
    // deleteChildren() after the MainWindow-derived part of this object is gone.
    // Their teardown emits signals wired to MainWindow slots that touch the
    // (dying) scene: the scene fires selectionChanged as its items are removed,
    // and ~QUndoStack calls clear() which fires indexChanged → refreshProperties.
    // Sever both now, while `this` is still a valid MainWindow.
    if (m_undoStack)
        m_undoStack->disconnect(this);
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

    auto *importPackAct = new QAction(tr("Import Object &Pack…"), this);
    connect(importPackAct, &QAction::triggered, this, &MainWindow::importObjectPack);
    addAction(importPackAct);

    auto *removePackAct = new QAction(tr("Remove Object Pac&k…"), this);
    connect(removePackAct, &QAction::triggered, this, &MainWindow::removeObjectPack);
    addAction(removePackAct);

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

    auto *prefsAct = new QAction(tr("&Preferences…"), this);
    prefsAct->setShortcut(QKeySequence::Preferences);  // ⌘, on macOS
    prefsAct->setMenuRole(QAction::PreferencesRole);   // moves to app menu on macOS
    connect(prefsAct, &QAction::triggered, this, &MainWindow::openPreferences);
    addAction(prefsAct);

    auto *exportPdfAct = new QAction(tr("&Export to PDF…"), this);
    exportPdfAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));
    connect(exportPdfAct, &QAction::triggered, this, &MainWindow::exportPdf);
    addAction(exportPdfAct);

    auto *quitAct = new QAction(tr("&Quit"), this);
    quitAct->setShortcut(QKeySequence::Quit);
    quitAct->setMenuRole(QAction::QuitRole);  // moves to app menu on macOS
    connect(quitAct, &QAction::triggered, this, &QWidget::close);
    addAction(quitAct);

    m_undoAction = m_undoStack->createUndoAction(this, tr("&Undo"));
    m_undoAction->setShortcuts(QKeySequence::Undo);
    addAction(m_undoAction);

    m_redoAction = m_undoStack->createRedoAction(this, tr("&Redo"));
    m_redoAction->setShortcuts(QKeySequence::Redo);
    addAction(m_redoAction);

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

    auto *resetLayoutAct = new QAction(tr("Reset &Window Layout"), this);
    connect(resetLayoutAct, &QAction::triggered, this, &MainWindow::resetWindowLayout);
    addAction(resetLayoutAct);

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
    fileMenu->addAction(byText(tr("Import Object &Pack…")));
    fileMenu->addAction(byText(tr("Remove Object Pac&k…")));
    fileMenu->addSeparator();
    fileMenu->addAction(byText(tr("&Save")));
    fileMenu->addAction(byText(tr("Save &As…")));
    fileMenu->addSeparator();
    fileMenu->addAction(byText(tr("Document &Info…")));
    fileMenu->addAction(byText(tr("&Export to PDF…")));
    fileMenu->addSeparator();
    fileMenu->addAction(byText(tr("&Quit")));

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(m_undoAction);
    editMenu->addAction(m_redoAction);
    editMenu->addSeparator();
    editMenu->addAction(byText(tr("&Delete")));
    editMenu->addAction(byText(tr("Select &All")));
    editMenu->addSeparator();
    editMenu->addAction(byText(tr("Rotate &Left")));
    editMenu->addAction(byText(tr("Rotate &Right")));
    editMenu->addSeparator();
    // PreferencesRole relocates this to the application menu on macOS.
    editMenu->addAction(byText(tr("&Preferences…")));

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(byText(tr("Zoom &In")));
    viewMenu->addAction(byText(tr("Zoom &Out")));
    viewMenu->addAction(byText(tr("&Actual Size")));
    viewMenu->addSeparator();
    viewMenu->addAction(byText(tr("Reset &Window Layout")));

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(byText(tr("&About stagePLTR")));
}

void MainWindow::createPalette()
{
    auto *dock = new QDockWidget(tr("Devices"), this);
    dock->setObjectName(QStringLiteral("DevicesDock"));  // for saveState/restoreState
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_palette = new DevicePalette(dock);
    m_palette->populate(m_catalog);
    dock->setWidget(m_palette);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    connect(m_palette, &DevicePalette::deviceActivated,
            this, &MainWindow::addDeviceAtCentre);
    connect(m_palette, &DevicePalette::objectContextMenu,
            this, &MainWindow::showObjectMenu);
}

void MainWindow::createPropertiesDock()
{
    auto *dock = new QDockWidget(tr("Properties"), this);
    dock->setObjectName(QStringLiteral("PropertiesDock"));
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_portEditor = new PortEditor(dock);
    dock->setWidget(m_portEditor);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    connect(m_portEditor, &PortEditor::edited, this, &MainWindow::handlePortEdit);
}

void MainWindow::createBreakoutDock()
{
    auto *dock = new QDockWidget(tr("Breakout / Input List"), this);
    dock->setObjectName(QStringLiteral("BreakoutDock"));
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

void MainWindow::refreshProperties()
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
    // Show the editor only when exactly one device is selected, and capture its
    // current label/ports as the baseline for the next edit's undo step.
    DeviceItem *bound = deviceCount == 1 ? device : nullptr;
    m_portEditor->setDevice(bound);
    m_editDevice = bound;
    m_editLabel = bound ? bound->label() : QString();
    m_editPorts = bound ? bound->ports() : QList<Port>();
}

void MainWindow::handleDrop(const QString &typeId, const QPointF &scenePos)
{
    m_undoStack->push(new AddDeviceCommand(m_scene, typeId, scenePos));
}

void MainWindow::handleTransforms(const QVector<DeviceTransform> &changes)
{
    bool moved = false;
    bool rotated = false;
    for (const DeviceTransform &c : changes) {
        moved = moved || c.newPos != c.oldPos;
        rotated = rotated || c.newRotation != c.oldRotation;
    }
    const QString text = moved      ? tr("Move Device(s)")
                         : rotated  ? tr("Rotate Device(s)")
                                    : tr("Move Label");
    m_undoStack->push(new TransformDevicesCommand(changes, text));
}

void MainWindow::handlePortEdit()
{
    if (!m_editDevice)
        return;
    const QString newLabel = m_editDevice->label();
    const QList<Port> newPorts = m_editDevice->ports();
    if (newLabel == m_editLabel && newPorts == m_editPorts)
        return;  // editor signalled but nothing actually changed

    // The editor already mutated the device; record the delta as one undo step
    // without re-binding the editor (which would interrupt the user's edit).
    m_suppressRefresh = true;
    m_undoStack->push(new EditDeviceCommand(m_scene, m_editDevice, m_editLabel,
                                            m_editPorts, newLabel, newPorts));
    m_suppressRefresh = false;
    m_editLabel = newLabel;
    m_editPorts = newPorts;
}

void MainWindow::newPlot()
{
    if (!maybeSave())
        return;
    m_scene->clearDevices();
    m_undoStack->clear();
    m_scene->setPageConfig(pageconfig::load(DocumentFeature::StagePlot));
    DocumentInfo info;
    info.date = QDate::currentDate();
    m_scene->setDocumentInfo(info);
    setCurrentFile(QString());
    updateWindowTitle();
    refreshProperties();
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
    const DocumentInfo oldInfo = m_scene->documentInfo();
    DocumentInfoDialog dialog(oldInfo, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const DocumentInfo newInfo = dialog.info();
    if (newInfo.toJson() != oldInfo.toJson())
        m_undoStack->push(new EditDocumentInfoCommand(m_scene, oldInfo, newInfo));
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
    QList<DeviceItem *> devices;
    for (QGraphicsItem *item : m_scene->selectedItems())
        if (item->type() == DeviceItem::Type)
            devices.append(static_cast<DeviceItem *>(item));
    if (!devices.isEmpty())
        m_undoStack->push(new RemoveDevicesCommand(m_scene, devices));
}

void MainWindow::rotateSelection(double degrees)
{
    QVector<DeviceTransform> changes;
    for (QGraphicsItem *item : m_scene->selectedItems()) {
        if (item->type() != DeviceItem::Type)
            continue;
        auto *device = static_cast<DeviceItem *>(item);
        DeviceTransform t;
        t.item = device;
        t.oldPos = t.newPos = device->pos();
        t.oldRotation = device->rotation();
        t.newRotation = device->rotation() + degrees;
        t.oldLabelOffset = t.newLabelOffset = device->labelOffset();
        changes.append(t);
    }
    if (!changes.isEmpty())
        m_undoStack->push(new TransformDevicesCommand(changes, tr("Rotate Device(s)")));
}

void MainWindow::addDeviceAtCentre(const QString &typeId)
{
    const QPointF centre = m_view->mapToScene(m_view->viewport()->rect().center());
    m_undoStack->push(new AddDeviceCommand(m_scene, typeId, centre));
}

void MainWindow::showObjectMenu(const QString &typeId, const QPoint &globalPos)
{
    if (!m_catalog.find(typeId))
        return;
    const bool isUser = m_catalog.isUserObject(typeId);

    QMenu menu(this);
    QAction *dup = menu.addAction(tr("Duplicate…"));
    QAction *edit = menu.addAction(tr("Edit…"));
    QAction *del = menu.addAction(tr("Delete"));
    edit->setEnabled(isUser);  // built-ins are read-only
    del->setEnabled(isUser);

    QAction *chosen = menu.exec(globalPos);
    if (chosen == dup)
        duplicateObject(typeId);
    else if (chosen == edit)
        editObject(typeId);
    else if (chosen == del)
        deleteObject(typeId);
}

QString MainWindow::makeUniqueObjectId(const QString &base) const
{
    QString candidate = base + QStringLiteral("-copy");
    for (int n = 2; m_catalog.find(candidate); ++n)
        candidate = base + QStringLiteral("-copy-") + QString::number(n);
    return candidate;
}

void MainWindow::duplicateObject(const QString &id)
{
    const DeviceType *src = m_catalog.find(id);
    if (!src)
        return;
    DeviceType copy = *src;
    copy.id = makeUniqueObjectId(src->id);
    copy.name = src->name + tr(" copy");
    copy.builtin = false;
    openObjectEditor(copy);
}

void MainWindow::editObject(const QString &id)
{
    const DeviceType *obj = m_catalog.find(id);
    if (obj && !obj->builtin)
        openObjectEditor(*obj);
}

void MainWindow::openObjectEditor(const DeviceType &seed)
{
    ObjectEditorDialog dialog(seed, m_catalog.orderedCategories(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    QString error;
    if (!m_catalog.addUserObject(dialog.object(), &error)) {
        QMessageBox::warning(this, tr("Save Object"), error);
        return;
    }
    m_palette->populate(m_catalog);
}

void MainWindow::deleteObject(const QString &id)
{
    const DeviceType *obj = m_catalog.find(id);
    if (!obj || obj->builtin)  // user objects only
        return;
    if (QMessageBox::question(
            this, tr("Delete Object"),
            tr("Remove \"%1\" from your object library?").arg(obj->name))
        != QMessageBox::Yes)
        return;
    QString error;
    if (!m_catalog.removeUserObject(id, &error)) {
        QMessageBox::warning(this, tr("Delete Object"), error);
        return;
    }
    m_palette->populate(m_catalog);
}

void MainWindow::openPreferences()
{
    PreferencesDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    // Apply the (possibly new) default to the current plot. The page setup is
    // also stored in the .splot, so this becomes part of the document.
    const PageConfig chosen = pageconfig::load(DocumentFeature::StagePlot);
    if (chosen != m_scene->pageConfig())
        m_undoStack->push(
            new ChangePageConfigCommand(m_scene, m_scene->pageConfig(), chosen));
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
    setWindowModified(!m_undoStack->isClean());
}

bool MainWindow::maybeSave()
{
    if (m_undoStack->isClean())
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
    root[QStringLiteral("info")] = m_scene->documentInfo().toJson();

    // Embed any custom (user) objects the plot uses, so it opens on any machine.
    QJsonArray customs;
    QSet<QString> seen;
    for (QGraphicsItem *gi : m_scene->items()) {
        if (gi->type() != DeviceItem::Type)
            continue;
        const QString typeId = static_cast<DeviceItem *>(gi)->typeId();
        if (seen.contains(typeId) || !m_catalog.isUserObject(typeId))
            continue;
        seen.insert(typeId);
        if (const DeviceType *t = m_catalog.find(typeId))
            customs.append(DeviceCatalog::toEmbeddedJson(*t));
    }
    if (!customs.isEmpty())
        root[QStringLiteral("customObjects")] = customs;

    const QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        QMessageBox::warning(this, tr("Save Failed"),
                             tr("Could not save %1").arg(path));
        return false;
    }
    setCurrentFile(path);
    m_undoStack->setClean();  // this state is now the saved one
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

    const QJsonObject root = doc.object();

    // Register any embedded custom objects (icons included) before parsing the
    // scene, so their placed devices resolve and render. Track the ones our
    // library doesn't already have, to offer importing them afterwards.
    QStringList newObjectIds;
    const QJsonArray customs = root.value(QStringLiteral("customObjects")).toArray();
    for (const QJsonValue &value : customs) {
        const DeviceType type = DeviceCatalog::fromEmbeddedJson(value.toObject());
        if (type.id.isEmpty() || m_catalog.find(type.id))
            continue;  // empty, or we already have it — keep our own copy
        m_catalog.addInMemoryObject(type);
        newObjectIds.append(type.id);
    }

    // Suppress live signals while we populate the scene programmatically.
    {
        const QSignalBlocker blocker(m_scene);
        m_scene->setDocumentInfo(
            DocumentInfo::fromJson(root.value(QStringLiteral("info")).toObject()));
        QString error;
        m_scene->fromJson(root, &error);
        if (!error.isEmpty())
            statusBar()->showMessage(error, 5000);
    }
    // Channel signals were blocked during load; sync the breakout view now.
    m_channelModel->setChannels(m_scene->channels());
    if (!newObjectIds.isEmpty())
        m_palette->populate(m_catalog);  // show the just-registered objects
    m_undoStack->clear();  // a freshly loaded document starts clean
    refreshProperties();

    setCurrentFile(path);
    updateWindowTitle();
    statusBar()->showMessage(tr("Opened %1").arg(QFileInfo(path).fileName()), 3000);

    offerToImportObjects(newObjectIds);
    return true;
}

void MainWindow::importObjectPack()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Import Object Pack"), QString(),
        tr("Object Pack (objects.json *.json);;All Files (*)"));
    if (path.isEmpty())
        return;

    QString packId, packName, error;
    if (!m_catalog.readPackHeader(path, &packId, &packName, nullptr, &error)) {
        QMessageBox::warning(this, tr("Import Object Pack"), error);
        return;
    }

    // Force a display name when the pack doesn't carry one — default to a unique
    // "Untitled N" so unnamed imports never collide on a blank name.
    if (packName.isEmpty()) {
        bool ok = false;
        packName = QInputDialog::getText(
                       this, tr("Name This Pack"),
                       tr("This pack has no name. Name it for your library:"),
                       QLineEdit::Normal, defaultPackName(), &ok)
                       .trimmed();
        if (!ok || packName.isEmpty())
            return;  // cancelled — abort the import
    }
    // Mint a stable identity when the file omits one (hand-authored packs), so
    // packs are tracked by GUID and same-named packs never merge.
    if (packId.isEmpty())
        packId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    QStringList added;
    const int count = m_catalog.importPack(path, packId, packName, &added, &error);
    if (count < 0) {
        QMessageBox::warning(this, tr("Import Object Pack"), error);
        return;
    }
    if (count == 0) {
        QMessageBox::information(
            this, tr("Import Object Pack"),
            tr("No objects could be imported from this pack (missing or "
               "unreadable icon files?)."));
        return;
    }

    m_palette->populate(m_catalog);  // surface the newly imported objects
    statusBar()->showMessage(
        tr("Imported %n object(s) into \"%1\"", "", count).arg(packName), 3000);
}

QString MainWindow::defaultPackName() const
{
    QSet<QString> used;
    for (const DeviceCatalog::PackInfo &p : m_catalog.importedPacks())
        used.insert(p.name);
    for (int n = 1; ; ++n) {
        const QString candidate = tr("Untitled %1").arg(n);
        if (!used.contains(candidate))
            return candidate;
    }
}

void MainWindow::removeObjectPack()
{
    const QList<DeviceCatalog::PackInfo> packs = m_catalog.importedPacks();
    if (packs.isEmpty()) {
        QMessageBox::information(this, tr("Remove Object Pack"),
                                 tr("Your library has no custom objects to remove."));
        return;
    }

    // Untagged objects (older imports, hand-made) collect under an "Ungrouped"
    // entry. Two packs can legitimately share a display name (different GUIDs),
    // so disambiguate those with a short id fragment to keep the chooser 1:1.
    QHash<QString, int> nameCount;
    for (const DeviceCatalog::PackInfo &p : packs)
        ++nameCount[p.name];

    QStringList labels;
    for (const DeviceCatalog::PackInfo &p : packs) {
        QString display = p.name.isEmpty() ? tr("Ungrouped objects") : p.name;
        if (!p.name.isEmpty() && nameCount.value(p.name) > 1 && !p.id.isEmpty())
            display += QStringLiteral(" · ") + p.id.left(8);
        labels.append(tr("%1  (%2)").arg(display).arg(p.count));
    }

    bool ok = false;
    const QString chosen = QInputDialog::getItem(
        this, tr("Remove Object Pack"),
        tr("Remove all objects from:"), labels, 0, /*editable=*/false, &ok);
    if (!ok)
        return;
    const int row = labels.indexOf(chosen);
    if (row < 0)
        return;
    const DeviceCatalog::PackInfo pack = packs.at(row);

    const QString warning = pack.name.isEmpty()
        ? tr("Remove all %n ungrouped object(s) from your library? This includes "
             "any objects you made by hand. This cannot be undone.", "", pack.count)
        : tr("Remove all %n object(s) in \"%1\" from your library? This cannot be undone.",
             "", pack.count).arg(pack.name);
    if (QMessageBox::question(this, tr("Remove Object Pack"), warning) != QMessageBox::Yes)
        return;

    QString error;
    const int removed = m_catalog.removePack(pack.id, pack.name, &error);
    if (removed < 0) {
        QMessageBox::warning(this, tr("Remove Object Pack"), error);
        return;
    }
    m_palette->populate(m_catalog);
    const QString display = pack.name.isEmpty() ? tr("ungrouped objects") : pack.name;
    statusBar()->showMessage(
        tr("Removed %n object(s) (%1)", "", removed).arg(display), 3000);
}

void MainWindow::offerToImportObjects(const QStringList &ids)
{
    if (ids.isEmpty())
        return;
    const auto choice = QMessageBox::question(
        this, tr("Custom Objects"),
        tr("This plot uses %1 custom object(s) not in your library.\n"
           "Add them to your library so you can reuse them?")
            .arg(ids.size()),
        QMessageBox::Yes | QMessageBox::No);
    if (choice != QMessageBox::Yes)
        return;
    for (const QString &id : ids)
        if (const DeviceType *type = m_catalog.find(id))
            m_catalog.addUserObject(*type);  // persist to the library
}

void MainWindow::setCurrentFile(const QString &path)
{
    m_currentFile = path;
    updateWindowTitle();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        saveWindowGeometry();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    // Centre on first show only — by now the window has its real frame, so the
    // centring accounts for the title bar.
    if (m_pendingCenter) {
        m_pendingCenter = false;
        centerOnScreen();
    }
}

QSize MainWindow::defaultWindowSize() const
{
    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen)
        screen = QGuiApplication::primaryScreen();

    QSize size(1280, 900);
    if (screen) {
        // Never exceed the usable area — important on small or scaled displays.
        const QSize avail = screen->availableGeometry().size();
        size.setWidth(qMin(size.width(), int(avail.width() * 0.9)));
        size.setHeight(qMin(size.height(), int(avail.height() * 0.9)));
    }
    return size;
}

void MainWindow::centerOnScreen()
{
    // Centre the window on the screen under the cursor. availableGeometry() and
    // frameGeometry() are both in logical points, so this is correct regardless
    // of the display's scale factor (no UHD-vs-scaled offset).
    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen)
        screen = this->screen();
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;

    const QRect avail = screen->availableGeometry();
    QRect frame = frameGeometry();
    frame.moveCenter(avail.center());
    move(frame.topLeft());
}

void MainWindow::restoreWindowGeometry()
{
    QSettings settings;
    const QByteArray geometry =
        settings.value(QStringLiteral("window/geometry")).toByteArray();
    if (!geometry.isEmpty() && restoreGeometry(geometry)) {
        restoreState(settings.value(QStringLiteral("window/state")).toByteArray());
    } else {
        // First run: a clamped default size, centred on the first show.
        resize(defaultWindowSize());
        m_pendingCenter = true;
    }
}

void MainWindow::saveWindowGeometry()
{
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("window/state"), saveState());
}

void MainWindow::resetWindowLayout()
{
    QSettings settings;
    settings.remove(QStringLiteral("window/geometry"));
    settings.remove(QStringLiteral("window/state"));

    restoreState(m_defaultLayout);  // the arrangement captured at startup
    resize(defaultWindowSize());
    centerOnScreen();
}