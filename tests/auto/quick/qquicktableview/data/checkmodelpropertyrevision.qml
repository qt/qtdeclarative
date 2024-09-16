// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.12

Item {
    id: root
    width: 640
    height: 480

    property alias tableView: tableView

    property int row: 42
    property int column: 42

    property int resolvedDelegateRow: 0
    property int resolvedDelegateColumn: 0

    TableView {
        id: tableView
        // Dummy tableView, to let the auto test follow the
        // same pattern for loading qml files as other tests.
    }

    Item {
        width: 100
        height: parent.height;
        Repeater {
            model: 1
            delegate: Component {
                Rectangle {
                    color: "blue"
                    height: 100
                    width: 100
                    Component.onCompleted: {
                        // row and column should be resolved to be the ones
                        // found in the root item, and not in the delegate
                        // items context. The context properties are revisioned,
                        // and require that the QQmlDelegateModel has an import
                        // version set (which is not the case when using a
                        // Repeater, only when using a TableView).
                        resolvedDelegateRow = row
                        resolvedDelegateColumn = column
                    }
                }
            }
        }
    }
}

