// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A child Rectangle is added BEFORE the font group property.
// Object indices are stable (adding an object doesn't renumber others), but
// the *contents* at each index change: index 1 was font-group in the old CU,
// now it's a Rectangle in the new CU, and the font-group moves to index 2.
// findInnerObjects hashes by objectIndex, so the entry for index 1 now has
// mismatched old (font/GroupProperty) and new (Rectangle/Object) sides.
import QtQuick

Text {
    Rectangle { objectName: "insertedChild"; width: 50 }
    font.pixelSize: 24
    text: "hello"
    property int marker: 2
}
