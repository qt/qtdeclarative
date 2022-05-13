// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick 2.9
import QtQuick.Window 2.2

Window {
    id: root
    visible: true
    width: 200
    height: 200
    property bool received: false
    Item {
        focus: true
        Shortcut {
            sequence: "B"
            onActivated: {
                root.received = true
            }
        }
    }
}

