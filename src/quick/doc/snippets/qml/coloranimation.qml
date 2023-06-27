// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Rectangle {
    width: 100; height: 100
    color: "red"

    ColorAnimation on color { to: "yellow"; duration: 1000 }
}
//![0]

