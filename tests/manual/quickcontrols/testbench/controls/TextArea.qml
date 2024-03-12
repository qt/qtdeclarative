// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
        ["disabled"],
    ]

    property Component component: Column {
        spacing: 10

        TextArea {
            text: "TextArea\nwith\ntext"
            enabled: !is("disabled")
        }

        TextArea {
            placeholderText: "TextArea with placeholderText"
            enabled: !is("disabled")
        }
    }
}
