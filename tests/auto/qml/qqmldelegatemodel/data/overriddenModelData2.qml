// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: root

    property AbstractItemModel model

    ListView {
        model: root.model
        delegate: Item {
            required property var modelData
            required property var model
        }
    }
}
