/*****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/
import QtQuick.Window 2.0
import QtQuick 2.0

Window {
    property Item containedObject: null
    property bool __resizeGuard: false
    onContainedObjectChanged:  {
        if (containedObject == undefined || containedObject == null) {
            visible = false;
            return;
        }
        __resizeGuard = true
        width = containedObject.width;
        height = containedObject.height;
        containedObject.parent = contentItem;
        visible = true;
        __resizeGuard = false
    }
    onWidthChanged: if (!__resizeGuard && containedObject) containedObject.width = width
    onHeightChanged: if (!__resizeGuard && containedObject) containedObject.height = height
}
