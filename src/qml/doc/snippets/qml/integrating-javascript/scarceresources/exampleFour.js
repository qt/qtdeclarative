// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
// exampleFour.js
.import Qt.example 1.0 as QtExample

function importAvatar() {
    var component = Qt.createComponent("exampleTwo.qml");
    var exampleOneElement = component.createObject(null);
    var avatarExample = exampleOneElement.a;
    var retn = avatarExample.avatar;
    retn.preserve();
    return retn;
}

function releaseAvatar(avatar) {
    avatar.destroy();
}
//![0]
