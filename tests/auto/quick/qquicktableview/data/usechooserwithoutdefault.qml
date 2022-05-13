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
                delegate: Item {
                    implicitWidth: 100
                    implicitHeight: 100
                }
            }
        }
    }
}
