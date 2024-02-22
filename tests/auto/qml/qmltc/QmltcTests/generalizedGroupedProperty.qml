// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml
import QtQuick
import QmltcTests

QmlGeneralizedGroupPropertyTestType {
    id: root

    property int myInt: 5

    group.count: myInt
    group.formula: 3 + 5
    group.str: "Hello World!"
    group.object: MyImmediateQtObject {
        property int myInt: 42
        root.group.count: myInt
        root.group.formula: 3 - 5
        root.group.str: "Goodbye World!"
    }
}
