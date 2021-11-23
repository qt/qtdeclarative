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

Dummy {
    id: root
    y : 10;
    width : 200;
    height : root.width;

    Dummy {
        id: child;
        height: 100;
        y: root.y + (root.height - child.height) / 2;
    }

    property string mass: {
        var result = "nothing";
        if (child.y > 100)
            result = "heavy";
        else
            result = "light";
        return result;
    }

    property real test_division: width / 1000 + 50;
    property real test_ternary: false ? 1 : true ? 2.2 : 1; // the type must be real

    property int test_switch: {
        switch (width % 3) {
            case 0:
                return 130;
            case 1:
                return 380;
            case 2:
                return 630;
        }
    }
}

