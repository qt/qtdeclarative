// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2

Image {
    width: 100
    height: 100
    source: "logo-big.jpg"
    Image {
        anchors.centerIn: parent
        source: "logo-small.jpg"
    }
}
