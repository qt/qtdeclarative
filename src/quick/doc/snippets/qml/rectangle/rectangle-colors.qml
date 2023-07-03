// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    width: 100; height: 200

Item {
    x: 10; y: 10
    width: 80; height: 180

//! [rectangles]
Rectangle {
    color: "#00B000"
    width: 80; height: 80
}

Rectangle {
    color: "steelblue"
    y: 100; width: 80; height: 80
}
//! [rectangles]
}
}
