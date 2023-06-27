// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
//![0]
Path {
    startX: 100; startY: 0

    PathArc {
        x: 0; y: 100
        radiusX: 100; radiusY: 100
        useLargeArc: true
    }
}
//![0]
