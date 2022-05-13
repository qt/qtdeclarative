// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
// exampleThree.js
.import Qt.example 1.0 as QtExample

var component = Qt.createComponent("exampleTwo.qml");
var exampleOneElement = component.createObject(null);
var avatarExample = exampleOneElement.a;
var retn = avatarExample.avatar;

// without the following call, the scarce resource held
// by retn would be automatically released by the engine
// after the import statement in exampleTwo.qml, prior
// to the variable assignment.
retn.preserve();

function importAvatar() {
    return retn;
}
//![0]
