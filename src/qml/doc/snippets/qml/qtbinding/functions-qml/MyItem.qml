// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
// MyItem.qml
import QtQuick

Item {
    function myQmlFunction(msg: string) : string {
        console.log("Got message:", msg)
        return "some return value"
    }
}
//![0]
