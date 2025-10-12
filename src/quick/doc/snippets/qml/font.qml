// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
//! [text]
    Text {
        font.family: "Helvetica"
        font.pointSize: 13
        font.bold: true
    }
//! [text]

//! [structured-value-construction]
    readonly property font myFont: ({
        family: "Helvetica",
        pointSize: 13,
        bold: true
    })
//! [structured-value-construction]
}
