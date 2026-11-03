// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml
import Test

BindablePoint {
    id: root

    property alias selfPoint: root.point
    property BindablePoint inner: BindablePoint { id: innerPoint }
    property alias innerPt: innerPoint.point
}
