// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

//DO NOT CHANGE

Item {
    id: item
    property int d: 0

    function doSomething() {
        var a = 5;
        var b = 6;
    }

    Component.onCompleted: doSomething()

}
