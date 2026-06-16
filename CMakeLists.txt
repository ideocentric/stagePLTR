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

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    devicetype.h
    devicecatalog.cpp
    devicecatalog.h
    deviceitem.cpp
    deviceitem.h
    stagescene.cpp
    stagescene.h
    stageview.cpp
    stageview.h
    devicepalette.cpp
    devicepalette.h
    ports.cpp
    ports.h
    pageconfig.cpp
    pageconfig.h
    preferencesdialog.cpp
    preferencesdialog.h
    channeltablemodel.cpp
    channeltablemodel.h
    porteditor.cpp
    porteditor.h
    pdfexporter.cpp
    pdfexporter.h
    documentinfo.cpp
    documentinfo.h
    documentinfodialog.cpp
    documentinfodialog.h
    resources.qrc        # Qt resource bundle for assets/ (plot symbols, UI glyphs)
)

qt_standard_project_setup()
qt_add_executable(stagePLTR
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

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