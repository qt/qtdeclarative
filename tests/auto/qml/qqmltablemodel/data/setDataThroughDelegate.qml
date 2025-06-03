// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12
import Qt.labs.qmlmodels 1.0

Item {
    id: root
    width: 200
    height: 200

    property alias testModel: testModel

    signal shouldModify()
    signal shouldModifyInvalidRole()
    signal shouldModifyInvalidType()

    function modify() {
        shouldModify()
    }

    function modifyInvalidRole() {
        shouldModifyInvalidRole()
    }

    function modifyInvalidType() {
        shouldModifyInvalidType()
    }

    TableView {
        anchors.fill: parent
        model: TestModel {
            id: testModel
        }

        delegate: Text {
            id: textItem
            text: model.display

            Connections {
                target: root
                enabled: column === 1
                function onShouldModify() { model.display = 18 }
            }

            Connections {
                target: root
                enabled: column === 0
                // Invalid: should be "display".
                function onShouldModifyInvalidRole() { model.age = 100 }
            }

            Connections {
                target: root
                enabled: column === 1
                // Invalid: should be an int.
                function onShouldModifyInvalidType() { model.display = "Whoops" }
            }
        }
    }
}
