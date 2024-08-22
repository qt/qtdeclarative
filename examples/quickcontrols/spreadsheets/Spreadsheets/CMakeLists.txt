# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(Spreadsheets LANGUAGES CXX)

qt_standard_project_setup(REQUIRES 6.8)
qt_add_library(${PROJECT_NAME} STATIC)

qt_add_qml_module(${PROJECT_NAME}
    URI ${PROJECT_NAME}
    VERSION 1.0
    QML_FILES
        Main.qml
        TableCell.qml
        HeaderToolBar.qml
        HelpDialog.qml
        ColumnHeaderView.qml
        RowHeaderView.qml
        HeaderViewTapHandler.qml
    SOURCES
        datamodel.h datamodel.cpp
        spreadcell.h spreadcell.cpp
        spreadformula.h spreadformula.cpp
        spreadkey.h
        spreadmimedataprovider.h spreadmimedataprovider.cpp
        spreadmodel.h spreadmodel.cpp
        spreadrole.h
    RESOURCES
        icons/insert_column_left.svg
        icons/insert_column_right.svg
        icons/insert_row_above.svg
        icons/insert_row_below.svg
        icons/remove_column.svg
        icons/remove_row.svg
        icons/paste.svg
        icons/copy.svg
        icons/cut.svg
        icons/help.svg
        icons/hide.svg
        icons/show.svg
        icons/reset_reordering.svg
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
)
