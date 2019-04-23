/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
