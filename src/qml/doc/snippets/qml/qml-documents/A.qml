// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [document]
// A.qml
import QtQuick

Item {
    id: root
    property string message: "From A"
    component MyInlineComponent : Item {
        Component.onCompleted: console.log(root.message)
    }
}
//! [document]
