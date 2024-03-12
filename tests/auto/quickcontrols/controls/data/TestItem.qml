// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: item
    property var createdCallback
    property var destroyedCallback
    Component.onCompleted: if (createdCallback) createdCallback(item)
    Component.onDestruction: if (destroyedCallback) destroyedCallback(item)
}
