// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [qmltc-hello-world-qml]
// HelloWorld.qml
import QtQml

QtObject {
    id: me
    property string hello: "Hello, qmltc!"

    function printHello(prefix: string, suffix: string) {
        console.log(prefix + me.hello + suffix);
    }

    signal created()
    Component.onCompleted: me.created();
}
//! [qmltc-hello-world-qml]
