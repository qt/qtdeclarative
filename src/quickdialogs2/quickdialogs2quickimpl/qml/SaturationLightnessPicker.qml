/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick.Dialogs
import QtQuick.Dialogs.quickimpl

SaturationLightnessPickerImpl {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0, contentItem.implicitWidth)
    implicitHeight: Math.max(background ? background.implicitHeight : 0, contentItem.implicitHeight)

    background: Rectangle {
        anchors.fill: parent
        color: control.visualFocus ? (control.pressed ? "#cce0ff" : "#f0f6ff") : (control.pressed ? "#d6d6d6" : "#f6f6f6")
        border.color: "#353637"
    }

    contentItem: ShaderEffect {
        scale: contentItem.width / width
        layer.enabled: true
        layer.smooth: true
        anchors.fill: parent

        property alias hue: control.hue

        fragmentShader: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/shaders/SaturationLightness.frag.qsb"
    }

    handle: PickerHandle {
        x: control.leftPadding + control.lightness * control.availableWidth - width / 2
        y: control.topPadding + (1.0 - control.saturation) * control.availableHeight - height / 2
        picker: control
        handleColor: control.color
        z: 1
    }
}
