/******************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt JavaScript to C++ compiler.
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
******************************************************************************/

pragma Strict
import QtQuick

Item {
    QtObject { id: object }
    Item { id: item }
    Rectangle { id: rectangle }
    Dummy { id: dummy }

    property QtObject objectAsObject: object as QtObject
    property QtObject objectAsItem: object as Item
    property QtObject objectAsRectangle: object as Rectangle
    property QtObject objectAsDummy: object as Dummy

    property QtObject itemAsObject: item as QtObject
    property QtObject itemAsItem: item as Item
    property QtObject itemAsRectangle: item as Rectangle
    property QtObject itemAsDummy: item as Dummy

    property QtObject rectangleAsObject: rectangle as QtObject
    property QtObject rectangleAsItem: rectangle as Item
    property QtObject rectangleAsRectangle: rectangle as Rectangle
    property QtObject rectangleAsDummy: rectangle as Dummy

    property QtObject dummyAsObject: dummy as QtObject
    property QtObject dummyAsItem: dummy as Item
    property QtObject dummyAsRectangle: dummy as Rectangle
    property QtObject dummyAsDummy: dummy as Dummy
}
