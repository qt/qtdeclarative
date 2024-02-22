// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import "global.js" as Program

Rectangle {
    width: 200; height: 200

    signal triggered
    signal incrementTriggered

    onTriggered: Program.doSomething();
    onIncrementTriggered: Program.doIncrement();
}
