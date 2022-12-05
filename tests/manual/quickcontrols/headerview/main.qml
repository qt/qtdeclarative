// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import QtQuick.Window
import Qt.labs.qmlmodels
import TestTableModelWithHeader

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("HeaderView Test")

    TestTableModelWithHeader {
        id: tableModel
        rowCount: 50
        columnCount: 80
    }

    TableView {
        id: tableView
        anchors.top: parent.top
        anchors.topMargin: horizontalHeader.height + rowSpacing
        anchors.left: parent.left
        anchors.leftMargin: verticalHeader.width + columnSpacing
        model: tableModel
        rightMargin: 100
        bottomMargin: 100
        columnSpacing: 4
        rowSpacing: 4
        syncDirection: Qt.Vertical | Qt.Horizontal
        implicitWidth: parent.width + columnSpacing
        implicitHeight: parent.height + rowSpacing
        clip: true
        delegate: Rectangle {
            implicitWidth: 150
            implicitHeight: 50
            color: "#e6ecf5"

            CheckBox {
                anchors.fill: parent
                text: model.display
                checked: model.edit
                leftPadding: 12
                onClicked: model.edit = checked
            }
        }
    }

    HorizontalHeaderView {
        id: horizontalHeader
        objectName: "horizontalHeader"
        anchors.top: parent.top
        anchors.left: tableView.left
        syncView: tableView
        clip: true
    }

    VerticalHeaderView {
        id: verticalHeader
        objectName: "verticalHeader"
        anchors.top: tableView.top
        syncView: tableView
        clip: true
    }

    ToolButton {
        width: verticalHeader.width
        height: horizontalHeader.height
        onClicked: {
            horizontalHeader.contentX = 0
            verticalHeader.contentY = 0
        }
    }
}
