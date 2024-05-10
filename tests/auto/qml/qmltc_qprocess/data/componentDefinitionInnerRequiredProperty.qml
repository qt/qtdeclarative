// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    Component {
        id: mycomp

        Item {
            Rectangle {
                // Inner required properties cannot be set so this
                // should produce an error
                required property bool bar
            }
        }
    }
}
