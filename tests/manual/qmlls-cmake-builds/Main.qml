// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import autoGenCMake

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    HelloWorld { myP: 55; myPPP: 55 }
}
