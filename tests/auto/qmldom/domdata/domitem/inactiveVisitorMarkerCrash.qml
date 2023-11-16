// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    HelloWorld { myP: 55; myPPP: 55 }
}
