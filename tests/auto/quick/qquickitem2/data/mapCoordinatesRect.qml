// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    id: root; objectName: "root"
    width: 200; height: 200

    Item { id: itemA; objectName: "itemA"; x: 50; y: 50; width: 25; height: 70 }

    Item {
        x: 50; y: 50
        rotation: 45
        Item { id: itemB; objectName: "itemB"; x: 100; y: 100; width: 30; height: 45 }
    }

    function mapAToB(x, y, w, h) {
        var pos = itemA.mapToItem(itemB, x, y, w, h)
        return Qt.rect(pos.x, pos.y, pos.width, pos.height)
    }

    function mapAToBRect(x, y, w, h) {
        var pos = itemA.mapToItem(itemB, Qt.rect(x, y, w, h))
        return Qt.rect(pos.x, pos.y, pos.width, pos.height)
    }

    function mapAFromB(x, y, w, h) {
        var pos = itemA.mapFromItem(itemB, x, y, w, h)
        return Qt.rect(pos.x, pos.y, pos.width, pos.height)
    }

    function mapAFromBRect(x, y, w, h) {
        var pos = itemA.mapFromItem(itemB, Qt.rect(x, y, w, h))
        return Qt.rect(pos.x, pos.y, pos.width, pos.height)
    }

    function mapAToNull(x, y, w, h) {
        var pos = itemA.mapToItem(null, x, y, w, h)
        return Qt.rect(pos.x, pos.y, pos.width, pos.height)
    }

    function mapAFromNull(x, y, w, h) {
        var pos = itemA.mapFromItem(null, x, y, w, h)
        return Qt.rect(pos.x, pos.y, pos.width, pos.height)
    }

    function checkMapAToInvalid(x, y, w, h) {
        try {
            itemA.mapToItem(1122, x, y, w, h)
        } catch (e) {
            return e instanceof TypeError
        }
        return false;
    }

    function checkMapAFromInvalid(x, y, w, h) {
        try {
            itemA.mapFromItem(1122, x, y, w, h)
        } catch (e) {
            return e instanceof TypeError
        }
        return false;
    }
}
