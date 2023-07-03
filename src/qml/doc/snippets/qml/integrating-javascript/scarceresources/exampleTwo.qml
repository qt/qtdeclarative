// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
// exampleTwo.qml
import QtQuick
import Qt.example

QtObject {
    property AvatarExample a;
    a: AvatarExample { id: example }
    property var avatar: example.avatar
    // Now `avatar' contains a reference to the scarce resource.
}
//![0]
