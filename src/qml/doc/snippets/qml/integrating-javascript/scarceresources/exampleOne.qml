// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
// exampleOne.qml
import QtQuick
import Qt.example

QtObject {
    property AvatarExample a;
    a: AvatarExample { id: example }
    property var avatarWidth: example.avatar,100
    // Here, we use "example.avatar,100" purely for illustration.
    // The value of `avatarWidth' will be 100 after evaluation.
    // E.g., you could imagine some js function which takes
    // an avatar, and returns the width of the avatar.
}
//![0]
