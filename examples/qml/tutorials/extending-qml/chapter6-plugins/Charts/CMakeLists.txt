# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt6_policy(SET QTP0001 NEW)
qt6_add_qml_module(chartsplugin
    URI "Charts"
    PLUGIN_TARGET chartsplugin
    DEPENDENCIES QtQuick
)

target_sources(chartsplugin PRIVATE
    piechart.cpp piechart.h
    pieslice.cpp pieslice.h
)

target_link_libraries(chartsplugin PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

install(TARGETS chartsplugin
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/Charts"
    LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}/Charts"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qmldir
    DESTINATION "${CMAKE_INSTALL_BINDIR}/Charts"
)
