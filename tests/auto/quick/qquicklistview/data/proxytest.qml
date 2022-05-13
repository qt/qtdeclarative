// Copyright (C) 2016 Canonical Limited and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.1
import Proxy 1.0

Item {

    ProxyTestInnerModel {
        id: innerModel
    }

    QSortFilterProxyModel {
        id: outerModel
        sourceModel: innerModel
        filterRegularExpression: RegExp("^H.*$")
    }

    width: 400
    height: 400
    ListView {
        anchors.fill: parent
        orientation: Qt.Vertical
        model: outerModel
        delegate: Rectangle {
            width: 400
            height: 50
            color: index % 2 ? "red" : "yellow"
        }
    }

    Timer {
        running: true
        interval: 500
        onTriggered: {
            innerModel.doStuff();
        }
    }
}
