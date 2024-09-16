// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.VectorImage
import SvgImageTest

Item {
    width: vectorImage.implicitWidth * (SvgManager.scale / 10.0)
    height: vectorImage.implicitHeight * (SvgManager.scale / 10.0)
    scale: SvgManager.scale / 10.0
    transformOrigin: Item.TopLeft
    VectorImage {
        id: vectorImage
        source: SvgManager.currentSource
    }
}
