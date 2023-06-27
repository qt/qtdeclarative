// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
//![0]
Path {
    startX: 50; startY: 100

    PathArc {
        x: 150; y: 100
        radiusX: 50; radiusY: 20
        xAxisRotation: 45
    }
}
//![0]
