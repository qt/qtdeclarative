/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
import QtQuick

Item {
    id: root
    property alias loop1: root.loop1
    property alias loop2: obj1.loop3
    property int p1: 1
    property Item i1: Item{
        id: it1
        property Item i11: Item{
            id: it11
            property int p1: 42
        }
    }
    QtObject {
        id: obj1
        property alias objRef2: obj2
        property real p1: 2.0
        property alias loop3: root.loop2
        property alias loop4: root.loop1
        property alias a3i: root.p1
        property alias a7i: obj2.a8i // loop error
    }
    QtObject {
        id: obj2
        property alias a8i: obj1.a3i
        property alias tooDeep: root.i1.i11.p1
        property alias invalid1: noId
        property alias invalid2: noId.objectName
        property alias a13i: obj1.objRef2.a8i // invalid property a8i error
    }
    property alias tooDeepRef: obj2.tooDeep
    Rectangle {
        color: "red"
        Text{
            id:t1
            text: obj1.p1
        }
        Text{
            text: obj2.a8i
            anchors.top: t1.bottom
        }
    }
}
