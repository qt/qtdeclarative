// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
// exampleThree.qml
import QtQuick
import Qt.example
import "exampleThree.js" as ExampleThreeJs

QtObject {
    property var avatar: ExampleThreeJs.importAvatar()
}
//![0]
