// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Window

Item {
    width: 640
    height: 450

    property alias tableView: tableView

    TableView {
        id: tableView
        width: 0
        height: 0
        delegate: tableViewDelegate

        Component.onCompleted: {
            positionViewAtCell(
                Qt.point(0,0),
                TableView.AlignHCenter,
                Qt.point(-5,-5)
            );
        }
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            objectName: "tableViewDelegate"
            implicitWidth: tableView.width
            implicitHeight: 50
        }
    }

}
