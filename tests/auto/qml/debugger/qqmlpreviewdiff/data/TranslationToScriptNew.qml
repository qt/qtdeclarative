// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml

QtObject {
    objectName: qsTr("Hello")
    property int aux: 1
    property string value: "prefix_" + aux
}
