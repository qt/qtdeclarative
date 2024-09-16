// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import ".."

Page {
    topPadding: Constants.pageTopPadding

    ColumnLayout {
        width: parent.width

        RowLayout {
            spacing: 40

            Switch {
                text: "Switch"
                Layout.fillWidth: false
            }

            Switch {
                text: "Checked Switch"
                checked: true
                Layout.fillWidth: false
            }

            Switch {
                text: "Disabled Switch"
                enabled: false
                Layout.fillWidth: false
            }

            Switch {
                text: "Checked disabled Switch"
                checked: true
                enabled: false
                Layout.fillWidth: false
            }

            Item {
                Layout.fillWidth: true
            }
        }

        RowLayout {
            spacing: 40

            Switch {
                text: "Switch"
                icon.source: Constants.iconSource
                Layout.fillWidth: false
            }

            Switch {
                text: "Checked Switch"
                icon.source: Constants.iconSource
                checked: true
                Layout.fillWidth: false
            }

            Switch {
                text: "Disabled Switch"
                icon.source: Constants.iconSource
                enabled: false
                Layout.fillWidth: false
            }

            Switch {
                text: "Checked disabled Switch"
                icon.source: Constants.iconSource
                checked: true
                enabled: false
                Layout.fillWidth: false
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }
}
