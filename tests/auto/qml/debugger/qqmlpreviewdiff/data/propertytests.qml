// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: root
    width: 400
    height: 400

    // P1: Add property
    property int intProp: 42

    // P2: Remove property (commented out)
    // property string removedProp: "remove me"

    // P3: Rename property
    property string renamedProp: "hello"

    // P4: Change property type
    property var typeChange: 10  // originally int, now var

    // P5: Change default value
    property int defaultValue: 1

    // P6: readonly property
    readonly property int readOnlyProp: 100

    // P7: Remove readonly
    property int readOnlyRemoved: 200

    // P8: required property
    required property int requiredProp

    // P9: remove required
    property int optionalProp

    // P10: final property
    final property int finalProp: 999

    // P11: remove final
    property int nonFinal: 111

    // P12: default property
    default property alias childrenItems: root.children

    // P14: change var to typed
    property int varToTyped: 50

    // P15: typed to var
    property var typedToVar: "string now"

    // P16: list property
    property list<int> listProp: [1,2,3]

    // P17: remove list property (commented out)
    // property list<int> removedList: [4,5,6]

    // P18: change list element type
    property list<string> listElementTypeChange: ["a","b"]

    // P19: add alias
    property alias aliasedProp: root.intProp

    // P25: alias target removed (commented)
    // property alias brokenAlias: removedProp
}
