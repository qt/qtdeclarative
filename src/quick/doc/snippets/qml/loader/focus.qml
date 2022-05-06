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
//![0]
import QtQuick 2.0

Rectangle {
    width: 200; height: 200

    Loader {
        id: loader
        focus: true
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            loader.source = "KeyReader.qml"
        }
    }

    Keys.onPressed: (event)=> {
        console.log("Captured:",
                    event.text);
    }
}
//![0]

