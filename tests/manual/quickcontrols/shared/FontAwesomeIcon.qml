// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import "qrc:/shared"

Text {
    id: icon
    property alias iconId: icon.text
    property alias size: icon.font.pixelSize
    font.family: FontAwesome.fontLoader.name
}
