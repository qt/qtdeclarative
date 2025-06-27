// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
// pragma ComponentBehavior: Bo

Item {
    id: outer

    Component {
        id: myComponent

        Rectangle {
            Component.onCompleted: {
                outer.width.toFixed() // externalId, field, method
            }
        }
    }
}
