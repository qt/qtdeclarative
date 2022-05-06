/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
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
******************************************************************************/

import QtQuick 2.0

//![0]
Item {
    width: 400
    height: 400

    Component {
        id: myComponent
        Text { text: modelIndex }    //okay
    }

    ListView {
        anchors.fill: parent
        model: 5
        delegate: Component {
            Loader {
                property int modelIndex: index
                sourceComponent: myComponent
            }
        }
    }
}
//![0]
