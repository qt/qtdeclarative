// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
//![0]
Path {
    startX: 0; startY: 100

    PathArc {
        relativeX: 50; y: 100
        radiusX: 25; radiusY: 15
    }
    PathArc {
        relativeX: 50; y: 100
        radiusX: 25; radiusY: 25
    }
    PathArc {
        relativeX: 50; y: 100
        radiusX: 25; radiusY: 50
    }
    PathArc {
        relativeX: 50; y: 100
        radiusX: 50; radiusY: 100
    }
}
//![0]
