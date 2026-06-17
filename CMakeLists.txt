cmake_minimum_required(VERSION 3.21)

project(stagePLTR
    VERSION 0.1.0
    DESCRIPTION "Stage plot and tech rider editor for bands"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt: auto-run moc, rcc, and uic on sources that need them.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Prefer Qt6; fall back to Qt5 if Qt6 isn't installed.
find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)

# Sources are grouped by responsibility under src/ (see folder names below).
set(PROJECT_SOURCES
    src/main.cpp

    # app shell — window, menus, wiring
    src/app/mainwindow.cpp
    src/app/mainwindow.h

    # canvas — the interactive stage scene/view/items
    src/canvas/stagescene.cpp
    src/canvas/stagescene.h
    src/canvas/stageview.cpp
    src/canvas/stageview.h
    src/canvas/deviceitem.cpp
    src/canvas/deviceitem.h

    # model — data types + (de)serialization (and shared paint helpers)
    src/model/channel.h
    src/model/devicetype.h
    src/model/devicecatalog.cpp
    src/model/devicecatalog.h
    src/model/ports.cpp
    src/model/ports.h
    src/model/pageconfig.cpp
    src/model/pageconfig.h
    src/model/documentinfo.cpp
    src/model/documentinfo.h
    src/model/inputlist.cpp
    src/model/inputlist.h

    # panels — docks, tables, dialogs
    src/panels/devicepalette.cpp
    src/panels/devicepalette.h
    src/panels/devicelist.cpp
    src/panels/devicelist.h
    src/panels/porteditor.cpp
    src/panels/porteditor.h
    src/panels/channeltablemodel.cpp
    src/panels/channeltablemodel.h
    src/panels/documentinfodialog.cpp
    src/panels/documentinfodialog.h
    src/panels/preferencesdialog.cpp
    src/panels/preferencesdialog.h

    # export — output renderers
    src/export/pdfexporter.cpp
    src/export/pdfexporter.h

    resources.qrc        # Qt resource bundle for assets/ (plot symbols, UI glyphs)
)

qt_standard_project_setup()
qt_add_executable(stagePLTR
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

# Add each source folder to the include path so headers stay addressable by a
# bare name (e.g. #include "ports.h") regardless of which folder they live in.
target_include_directories(stagePLTR PRIVATE
    src
    src/app
    src/canvas
    src/model
    src/panels
    src/export
)

# Mirror the on-disk folders in IDE project trees (CLion / Qt Creator).
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJECT_SOURCES})

target_link_libraries(stagePLTR PRIVATE
    Qt6::Widgets
    Qt6::Svg      # renders SVG plot symbols / UI glyphs
)

# Platform bundle/packaging properties.
set_target_properties(stagePLTR PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER com.ideocentric.stagePLTR
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS stagePLTR
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_finalize_executable(stagePLTR)