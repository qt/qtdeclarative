/******************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Ultralite module.
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
import QtQuick 2.15

Item {
    property real cost1: fruitModel.get(1).cost
    property string name1: fruitModel.get(1).name
    property real cost2: fruitModel.get(2).cost
    property string name2: fruitModel.get(2).name
    ListModel {
        id: fruitModel

        ListElement {
            name: "Apple"
            cost: 2.2
        }
        ListElement {
            name: "Orange"
            cost: 3
        }
        ListElement {
            name: "Banana"
            cost: 1.95
        }
    }
    Component.onCompleted: {
        console.log(fruitModel.data(fruitModel.index(1, 0), 0))
        console.log(fruitModel.get(0).name)
    }
}
