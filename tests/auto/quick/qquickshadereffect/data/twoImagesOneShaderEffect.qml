// Copyright (C) 2016 Canonical Limited and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.4

Item {
    width: 400
    height: 700

    MyIcon {
        id: icon

        height: 24
        source: "star.png"
        shaderActive: true
    }

    MyIcon {
        anchors.top: icon.bottom

        height: 24
        source: "star.png"
    }
}
