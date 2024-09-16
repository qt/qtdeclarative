// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Item {
    width: 200
    height: 200

    Image {
        source: "toowide.png"
        width: 100
        height: 100
    }

    Image {
        x: 100
        source: "tootall.png"
        width: 100
        height: 100
    }
}
