// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQml 2.1
import QtQuick 2.2
import Qt.labs.settings 1.0
import Qt.test 1.0

CppObject {
    id: obj

    property Settings settings: Settings {
        property alias intProperty: obj.intProperty
        property alias boolProperty: obj.boolProperty
        property alias realProperty: obj.realProperty
        property alias doubleProperty: obj.doubleProperty
        property alias stringProperty: obj.stringProperty
        property alias urlProperty: obj.urlProperty
        property alias objectProperty: obj.objectProperty
        property alias intListProperty: obj.intListProperty
        property alias stringListProperty: obj.stringListProperty
        property alias objectListProperty: obj.objectListProperty
        property alias dateProperty: obj.dateProperty
        // QTBUG-32295: Expected property type
        //property alias timeProperty: obj.timeProperty
        property alias sizeProperty: obj.sizeProperty
        property alias pointProperty: obj.pointProperty
        property alias rectProperty: obj.rectProperty
        property alias colorProperty: obj.colorProperty
        property alias fontProperty: obj.fontProperty
    }
}
