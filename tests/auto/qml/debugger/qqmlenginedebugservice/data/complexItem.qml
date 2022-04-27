/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import Test 1.0
Item {
    id: root
    width: 10; height: 20; scale: blueRect.scale;
    Rectangle { id: blueRect; width: 500; height: 600; color: "blue"; }
    Text { font.bold: true; color: blueRect.color; }
    MouseArea {
        onEntered: { console.log('hello') }
    }
    property variant varObj
    property variant varObjList: []
    property variant varObjMap
    property variant simpleVar: 10.05
    Component.onCompleted: {
        varObj = blueRect;
        var list = varObjList;
        list[0] = blueRect;
        varObjList = list;
        var map = new Object;
        map.rect = blueRect;
        varObjMap = map;
    }
    NonScriptPropertyElement {
    }
}
