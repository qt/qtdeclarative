// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
//![0]
Rectangle {
    property alias color: childRect.color
    color: "red"

    Rectangle { id: childRect }
}
//![0]
