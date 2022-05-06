/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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
******************************************************************************/

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.Imagine
import QtQuick.Controls.Imagine.impl

T.SelectionRectangle {
    id: control

    topLeftHandle: handle
    bottomRightHandle: handle

    Component {
        id: handle
        Image {
            id: image
            source: Imagine.url + "slider-handle"
            visible: SelectionRectangle.control.active
            ImageSelector on source {
                states: [
                    {"vertical": false},
                    {"horizontal": true},
                    {"disabled": false},
                    {"pressed": tapHandler.pressed || image.SelectionRectangle.dragging},
                    {"focused": true},
                    {"mirrored": false},
                    {"hovered": hoverHandler.hovered}
                ]
            }

            HoverHandler {
                id: hoverHandler
            }

            TapHandler  {
                id: tapHandler
            }
        }
    }
}
