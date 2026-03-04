// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    width: 640
    height: 450

    property alias tableView: tableView

    TableView {
        id: tableView

        anchors.fill: parent

        delegate: DelegateChooser {
            role: "choice"

            DelegateChoice {
                roleValue: 0

                delegate: Rectangle {
                    implicitWidth: 50
                    implicitHeight: 50
                    color: model.choice0Data

                    Component.onCompleted: tableView.model.delegate_created(0)
                }
            }

            DelegateChoice {
                roleValue: 1
                delegate: Text {
                    text: model.choice1Data

                    Component.onCompleted: tableView.model.delegate_created(1)
                }
            }
        }
    }
}
