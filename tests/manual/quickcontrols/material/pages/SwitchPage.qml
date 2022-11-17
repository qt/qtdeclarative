// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
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
            }

            Switch {
                text: "Checked Switch"
                checked: true
            }

            Switch {
                text: "Disabled Switch"
                enabled: false
            }

            Switch {
                text: "Checked disabled Switch"
                checked: true
                enabled: false
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
            }

            Switch {
                text: "Checked Switch"
                icon.source: Constants.iconSource
                checked: true
            }

            Switch {
                text: "Disabled Switch"
                icon.source: Constants.iconSource
                enabled: false
            }

            Switch {
                text: "Checked disabled Switch"
                icon.source: Constants.iconSource
                checked: true
                enabled: false
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }
}
