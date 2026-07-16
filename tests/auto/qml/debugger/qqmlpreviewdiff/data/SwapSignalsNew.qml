// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    property int marker: 0
    signal beta()
    signal alpha()
    onAlpha: marker = 1
    onBeta: marker = 2
}
