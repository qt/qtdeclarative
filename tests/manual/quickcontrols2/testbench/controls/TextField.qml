// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
        ["disabled"],
    ]

    property Component component: Column {
        spacing: 10

        TextField {
            text: "TextField with text"
            enabled: !is("disabled")
        }

        TextField {
            placeholderText: "TextField with placeholderText"
            enabled: !is("disabled")
        }
    }
}
