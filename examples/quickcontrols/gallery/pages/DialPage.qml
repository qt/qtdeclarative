// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ScrollablePage {
    id: page

    Column {
        spacing: 40
        width: parent.width

        Label {
            width: parent.width
            wrapMode: Label.Wrap
            horizontalAlignment: Qt.AlignHCenter
            text: "The Dial is similar to a traditional dial knob that is found on devices such as "
                + "stereos or industrial equipment. It allows the user to specify a value within a range."
        }

        Dial {
            value: 0.5
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
