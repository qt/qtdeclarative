// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: root;

    property string fontName: {
        if (Qt.platform.os === "osx")
            return "Helvetica"
        else if (Qt.platform.os === "linux")
            return "Open Sans"
        else if (Qt.platform.os === "windows")
            return "Arial"
        else
            return "Comic Sans"
    }

    Repeater {
        model: 100
        delegate: Item {
            TextMetrics {
                font.family: root.fontName
                text: "Hello world"
            }
        }
    }
}
