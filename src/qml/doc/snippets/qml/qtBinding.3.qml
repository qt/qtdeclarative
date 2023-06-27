// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
Item {
    id: root
    property string dynamicText: "Root text"

    Loader {
        id: loaderOne
        onLoaded: root.dynamicText = "Modified root text"
    }

    Loader {
        id: loaderTwo
        onLoaded: item.dynamicText = "Modified dynamic text"
    }

    Component.onCompleted: {
        loaderOne.setSource("DynamicText.qml", { 'text': Qt.binding(function() { return dynamicText + ' extra text' }) })
        loaderTwo.setSource("DynamicText.qml", { 'text': Qt.binding(function() { return this.dynamicText + ' extra text' }) })
    }
}
//![0]
