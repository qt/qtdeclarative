// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
        ["disabled"]
    ]

    property Component component: Label {
        text: "Label with a <a href=\"http://doc.qt.io\">link</a>"
        onTextChanged: print(text)
        enabled: !is("disabled")
        textFormat: Label.StyledText
    }
}
