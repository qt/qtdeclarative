// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Shape {
    id: item
    preferredRendererType: mainWindow.preferredRendererType
    property string svg_id
    containsMode: Shape.FillContains
}
