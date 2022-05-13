// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.3
import Qt.labs.qmlmodels 1.0

Item {
    width: 640
    height: 450

    property alias tableView: tableView

    TableView {
        id: tableView
        width: 600
        height: 400
        delegate: DelegateChooser {
            DelegateChoice {
                row: 0
                column: 0
                delegate: maskDelegate
            }
            DelegateChoice {
                row: 1
                column: 1
                delegate: maskDelegate
            }
            DelegateChoice {
                delegate: tableViewDelegate
            }
        }
    }

    Component {
        // Add this mask delegate, to force QQmlTableInstanceModel to
        // reuse the precise cells that we want to swap in the test
        id: maskDelegate
        Rectangle {
            implicitWidth: 100
            implicitHeight: 50
            color: "green"
        }
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            implicitWidth: 100
            implicitHeight: 50
            Text {
                anchors.fill: parent
                text: column + "," + row
            }
        }
    }

}
