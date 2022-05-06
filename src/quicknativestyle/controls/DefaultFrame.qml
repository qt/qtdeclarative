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
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.NativeStyle as NativeStyle

T.Frame {
    id: control

    readonly property bool __nativeBackground: background instanceof NativeStyle.StyleItem

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    leftPadding: __nativeBackground ? background.contentPadding.left : 12
    rightPadding: __nativeBackground ? background.contentPadding.right : 12
    topPadding: __nativeBackground ? background.contentPadding.top : 12
    bottomPadding: __nativeBackground ? background.contentPadding.bottom : 12

    background: NativeStyle.Frame {
        control: control
        contentWidth: control.contentWidth
        contentHeight: control.contentHeight
    }
}
