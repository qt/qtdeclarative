// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QmltcTests 1.0

TypeForCustomInitialization {
    id: myWindow
    required property int someValue

    property alias someValueAlias: myWindow.someValue
    property int someValueBinding: someValue + 1

    property bool wasSomeValueChanged: false

    property int someComplexValueThatWillBeSet: { return 5 }
    property int someComplexValueThatWillNotBeSet: { return 5 }

    property list<int> valueTypeList : []
    property list<Item> objectTypeList : []

    //QTBUG-114403: onValueChanged should not trigger when setting
    //the initial values.
    onSomeValueChanged: { wasSomeValueChanged = true; }
}
