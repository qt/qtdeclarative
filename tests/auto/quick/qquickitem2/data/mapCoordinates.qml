// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    id: root; objectName: "root"
    width: 200; height: 200

    Item { id: itemA; objectName: "itemA"; x: 50; y: 50 }

    Item {
        x: 50; y: 50
        Item { id: itemB; objectName: "itemB"; x: 100; y: 100 }
    }

    Component {
        id: itemComponent
        Item { x: 150; y: 150 }
    }

    function mapAToB(x, y) {
        var pos = itemA.mapToItem(itemB, x, y)
        return Qt.point(pos.x, pos.y)
    }

    function mapAToBPoint(x, y) {
        var pos = itemA.mapToItem(itemB, Qt.point(x, y))
        return Qt.point(pos.x, pos.y)
    }

    function mapAFromB(x, y) {
        var pos = itemA.mapFromItem(itemB, x, y)
        return Qt.point(pos.x, pos.y)
    }

    function mapAFromBPoint(x, y) {
        var pos = itemA.mapFromItem(itemB, Qt.point(x, y))
        return Qt.point(pos.x, pos.y)
    }

    function mapAToNull(x, y) {
        var pos = itemA.mapToItem(null, x, y)
        return Qt.point(pos.x, pos.y)
    }

    function mapAFromNull(x, y) {
        var pos = itemA.mapFromItem(null, x, y)
        return Qt.point(pos.x, pos.y)
    }

    function mapAToGlobal(x, y) {
        var pos = itemA.mapToGlobal(x, y)
        return Qt.point(pos.x, pos.y)
    }

    function mapAToGlobalPoint(x, y) {
        var pos = itemA.mapToGlobal(Qt.point(x, y))
        return Qt.point(pos.x, pos.y)
    }

    function mapAFromGlobal(x, y) {
        var pos = itemA.mapFromGlobal(x, y)
        return Qt.point(pos.x, pos.y)
    }

    function mapAFromGlobalPoint(x, y) {
        var pos = itemA.mapFromGlobal(Qt.point(x, y))
        return Qt.point(pos.x, pos.y)
    }

    function mapOrphanToGlobal(x, y) {
        var obj = itemComponent.createObject(null);
        var pos = obj.mapToGlobal(x, y)
        return Qt.point(pos.x, pos.y)
    }

    function mapOrphanFromGlobal(x, y) {
        var obj = itemComponent.createObject(null);
        var pos = obj.mapFromGlobal(x, y)
        return Qt.point(pos.x, pos.y)
    }

    function checkMapAToInvalid(x, y) {
        try {
            itemA.mapToItem(1122, x, y)
        } catch (e) {
            return e instanceof TypeError
        }
        return false
    }

    function checkMapAFromInvalid(x, y) {
        try {
            itemA.mapFromItem(1122, x, y)
        } catch (e) {
            return e instanceof TypeError
        }
        return false
    }
}
