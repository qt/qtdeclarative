// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick
import "script.js" as MyScript

Item {
    id: item
    width: 200; height: 200

    TapHandler {
        id: inputHandler
    }

    Component.onCompleted: {
        inputHandler.tapped.connect(MyScript.jsFunction)
    }
}
//![0]
