// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick.Window 2.0
import QtQuick 2.0

Window {
    property Item containedObject: null
    onContainedObjectChanged:  {
        if (containedObject == undefined || containedObject == null) {
            visible = false;
            return;
        }
        width = containedObject.width;
        height = containedObject.height;
        containedObject.parent = contentItem;
        visible = true;
    }
    onWidthChanged: if (containedObject) containedObject.width = width
    onHeightChanged: if (containedObject) containedObject.height = height
}
