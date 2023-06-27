// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
//![0]
Path {
    startX: 0; startY: 100
    PathArc {
        x: 100; y: 200
        radiusX: 100; radiusY: 100
        direction: PathArc.Clockwise
    }
}
//![0]
