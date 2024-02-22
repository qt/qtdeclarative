// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {

    function checkParameters(a: int, b: double, {x, y={}, z=[x,y]}) {
        return a + b + c + x + y + z
    }

    function deconstructingUsages(xxx) {
        let {a, b} = xxx;
        let c = a + b;
    }
}
