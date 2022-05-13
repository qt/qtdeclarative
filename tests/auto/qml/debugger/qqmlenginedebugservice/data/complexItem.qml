// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
