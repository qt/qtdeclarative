// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    property int base: 1
    property int other: 2
    property string marker: ""
    onBaseChanged: marker = "base"
    onOtherChanged: marker = "other"
}
