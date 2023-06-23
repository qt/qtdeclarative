# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(highdpi)
# add_subdirectory(httpserver) # TODO: C++ API changed, does this test still make sense?
add_subdirectory(listview)
add_subdirectory(mousearea)
add_subdirectory(nodetypes_ng)
# add_subdirectory(qmltypememory) # TODO: C++ API changed
# add_subdirectory(quickwidgetviewer) # TODO: C++ API changed
add_subdirectory(righttoleft)
add_subdirectory(scalablepath)
add_subdirectory(shapestest)
add_subdirectory(tableview)
add_subdirectory(text)
add_subdirectory(touch)
add_subdirectory(threading)
# add_subdirectory(v4) # TODO: port if needed
add_subdirectory(quickcontrols)
add_subdirectory(quickdialogs)
add_subdirectory(frameanimation)
if(QT_FEATURE_qml_network AND TARGET Qt::Quick)
    add_subdirectory(networkaccessmanagerfactory)
endif()
add_subdirectory(qmlextensionplugins)
add_subdirectory(fontfeatures)
