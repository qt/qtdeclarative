// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQml
import QtCore
import QtQuick

QtObject {
    id: root

    property int intProperty
    property bool boolProperty
    property real realProperty
    property double doubleProperty
    property string stringProperty
    property url urlProperty
    property var objectProperty
    property var intListProperty
    property var stringListProperty
    property var objectListProperty
    property date dateProperty
    // QTBUG-32295: Expected property type
    // property time timeProperty
    property size sizeProperty
    property point pointProperty
    property rect rectProperty
    property color colorProperty
    property font fontProperty

    function readSettings() {
        __setProperties(settings, root)
    }

    function writeSettings() {
        __setProperties(root, settings)
    }

    function __setProperties(from, to) {
        to.intProperty = from.intProperty
        to.boolProperty = from.boolProperty
        to.realProperty = from.realProperty
        to.doubleProperty = from.doubleProperty
        to.stringProperty = from.stringProperty
        to.urlProperty = from.urlProperty
        to.objectProperty = from.objectProperty
        to.intListProperty = from.intListProperty
        to.stringListProperty = from.stringListProperty
        to.objectListProperty = from.objectListProperty
        to.dateProperty = from.dateProperty
        //to.timeProperty = from.timeProperty
        to.sizeProperty = from.sizeProperty
        to.pointProperty = from.pointProperty
        to.rectProperty = from.rectProperty
        to.colorProperty = from.colorProperty
        to.fontProperty = from.fontProperty
    }

    property Settings settings: Settings {
        id: settings

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
    }
}
