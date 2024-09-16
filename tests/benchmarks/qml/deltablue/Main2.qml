// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml

import "deltablue.js" as DeltaBlue

QtObject {
    property var chain: DeltaBlue.chainTest(100)
    property var projection: DeltaBlue.projectionTest(100);

    function deltaBlue() {
        chain();
        projection();
    }
}
