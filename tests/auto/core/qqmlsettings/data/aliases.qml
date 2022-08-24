// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQml
import QtCore
import QtQuick

QtObject {
    id: root

    property int intProperty: 123
    property bool boolProperty: true
    property real realProperty: 1.23
    property double doubleProperty: 3.45
    property string stringProperty: "foo"
    property url urlProperty: "http://www.qt-project.org"
    property var objectProperty: {"foo":"bar"}
    property var intListProperty: [1, 2, 3]
    property var stringListProperty: ["a", "b", "c"]
    property var objectListProperty: [{"a":"b"}, {"c":"d"}]
    property date dateProperty: "2000-01-02"
    // QTBUG-32295: Expected property type
    //property time timeProperty: "12:34:56"
    property size sizeProperty: Qt.size(12, 34)
    property point pointProperty: Qt.point(12, 34)
    property rect rectProperty: Qt.rect(1, 2, 3, 4)
    property color colorProperty: "red"
    property font fontProperty

    property Settings settings: Settings {
        id: settings

        property alias intProperty: root.intProperty
        property alias boolProperty: root.boolProperty
        property alias realProperty: root.realProperty
        property alias doubleProperty: root.doubleProperty
        property alias stringProperty: root.stringProperty
        property alias urlProperty: root.urlProperty
        property alias objectProperty: root.objectProperty
        property alias intListProperty: root.intListProperty
        property alias stringListProperty: root.stringListProperty
        property alias objectListProperty: root.objectListProperty
        property alias dateProperty: root.dateProperty
        // QTBUG-32295: Expected property type
        //property alias timeProperty: root.timeProperty
        property alias sizeProperty: root.sizeProperty
        property alias pointProperty: root.pointProperty
        property alias rectProperty: root.rectProperty
        property alias colorProperty: root.colorProperty
        property alias fontProperty: root.fontProperty
    }
}
