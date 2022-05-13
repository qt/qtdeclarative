// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick

Item {
    width: 640
    height: 450

    property alias tableView: tableView
    property real delegateWidth: 100
    property real delegateHeight: 50
    property Component delegate: tableViewDelegate
    property bool delegateParentSetBeforeCompleted: false

    TableView {
        id: tableView
        width: 600
        height: 400
        anchors.margins: 1
        clip: true
        delegate: tableViewDelegate
        columnSpacing: 1
        rowSpacing: 1
        animate: false
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            objectName: "tableViewDelegate"
            implicitWidth: delegateWidth
            implicitHeight: delegateHeight
            color: "lightgray"
            border.width: 1

            property string modelDataBinding: modelData

            Text {
                anchors.centerIn: parent
                text: modelData
            }

            Component.onCompleted: {
                delegateParentSetBeforeCompleted = parent != null;
            }
        }
    }

}
