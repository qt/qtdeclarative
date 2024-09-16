// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQml 2.14

Rectangle {
    height: 400
    width: 400

    Rectangle {
        property bool when: true

        id: myItem
        objectName:  "myItem"
        height: 300
        width: 200
        property var foo: 42
    }

    property var boundValue: 13

    Binding {
        restoreMode: Binding.RestoreBindingOrValue
        objectName: "theBinding"
        target: myItem
        property: "foo"
        when: myItem.when
        value: boundValue
    }
}
