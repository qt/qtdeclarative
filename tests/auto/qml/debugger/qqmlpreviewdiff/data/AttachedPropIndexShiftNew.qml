// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A child Rectangle is added BEFORE the Keys attached property.
// The content at the Keys object's old index now holds the Rectangle;
// the Keys attached object moves to a new index.  Same mismatch pattern
// as GroupPropIndexShift but for an attached property.
import QtQuick

Item {
    Rectangle { objectName: "insertedChild"; width: 50 }
    Keys.enabled: false
    property int marker: 2
}
