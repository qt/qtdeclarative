// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//! [rectangle]
Rectangle {
    x: rectPosition()
    y: rectPosition()
    width: 200
    height: 200
    color: "lightblue"

    function rectPosition() {
        return enabled ? 0 : 100
    }
}
//! [rectangle]
