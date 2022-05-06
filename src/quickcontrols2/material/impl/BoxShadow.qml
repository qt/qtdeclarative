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
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl

/*
   A implementation of CSS's box-shadow, used by ElevationEffect for a Material Design
   elevation shadow effect.
 */
RectangularGlow {
    // The 4 properties from CSS box-shadow, plus the inherited color property
    property int offsetX
    property int offsetY
    property int blurRadius
    property int spreadRadius

    // The source item the shadow is being applied to, used for correctly
    // calculating the corner radious
    property Item source

    property bool fullWidth
    property bool fullHeight

    x: (parent.width - width)/2 + offsetX
    y: (parent.height - height)/2 + offsetY

    implicitWidth: source ? source.width : parent.width
    implicitHeight: source ? source.height : parent.height

    width: implicitWidth + 2 * spreadRadius + (fullWidth ? 2 * cornerRadius : 0)
    height: implicitHeight + 2 * spreadRadius + (fullHeight ? 2 * cornerRadius : 0)
    glowRadius: blurRadius/2
    spread: 0.05
    // qmllint disable unqualified
    // Intentionally duck-typed (QTBUG-94807)
    cornerRadius: blurRadius + (source && source.radius || 0)
}
