// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.13
import Qt.labs.qmlmodels 1.0

Item {
    id: root
    width: 200
    height: 200

    property alias testModel: testModel
    property alias tableView: tableView

    function appendRow(personName, personAge) {
        testModel.appendRow({
            name: personName,
            age: personAge
        })
    }

    function appendRowExtraData() {
        testModel.appendRow({
            name: "Foo",
            age: 99,
            nonExistentRole: 123
        })
    }

    function appendRowInvalid1() {
        testModel.appendRow(123)
    }

    function appendRowInvalid2() {
        testModel.appendRow({
            name: "Foo",
            age: []
        })
    }

    function appendRowInvalid3() {
        testModel.appendRow([
            { name: "Bar" },
            { age: "111" }
        ])
    }

    function insertRow(personName, personAge, rowIndex) {
        testModel.insertRow(rowIndex, {
            name: personName,
            age: personAge
        })
    }

    function insertRowExtraData() {
        testModel.insertRow(0, {
            name: "Foo",
            age: 99,
            nonExistentRole: 123
        })
    }

    function insertRowInvalid1() {
        testModel.insertRow(0, 123)
    }

    function insertRowInvalid2() {
        testModel.insertRow(0, {
            name: "Foo",
            age: []
        })
    }

    function insertRowInvalid3() {
        testModel.insertRow(0, [
            { name: "Bar" },
            { age: "111" }
        ])
    }

    function setRow(rowIndex, personName, personAge) {
        testModel.setRow(rowIndex, {
            name: personName,
            age: personAge
        })
    }

    function setRowExtraData() {
        testModel.setRow(0, {
            name: "Foo",
            age: 99,
            nonExistentRole: 123
        })
    }

    function setRowInvalid1() {
        testModel.setRow(0, 123)
    }

    function setRowInvalid2() {
        testModel.setRow(0, {
            name: "Foo",
            age: []
        })
    }

    function setRowInvalid3() {
        testModel.setRow(0, [
            { name: "Bar" },
            { age: "111" }
        ])
    }

    TableView {
        id: tableView
        anchors.fill: parent
        model: TestModel {
            id: testModel
        }
        delegate: Text {
            text: model.display
        }
    }
}
