// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    property int b
    property int a
    property string marker: ""
    signal ping()
    onPing: marker = "pinged"
}
