// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
Item {
    id: root
    property string dynamicText: "Root text"

    Component.onCompleted: {
        var c = Qt.createComponent("DynamicText.qml")

        var obj1 = c.createObject(root, { 'text': Qt.binding(function() { return dynamicText + ' extra text' }) })
        root.dynamicText = "Modified root text"

        var obj2 = c.createObject(root, { 'text': Qt.binding(function() { return this.dynamicText + ' extra text' }) })
        obj2.dynamicText = "Modified dynamic text"
    }
}
//![0]
