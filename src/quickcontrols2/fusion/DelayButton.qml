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
import QtQuick.Controls.Fusion
import QtQuick.Controls.Fusion.impl

T.DelayButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 6

    transition: Transition {
        NumberAnimation {
            duration: control.delay * (control.pressed ? 1.0 - control.progress : 0.3 * control.progress)
        }
    }

    contentItem: ItemGroup {
        ClippedText {
            clip: control.progress > 0
            clipX: -control.leftPadding + (control.mirrored ? 0 : control.progress * control.width)
            clipWidth: control.width
            visible: control.mirrored ? control.progress > 0 : control.progress < 1

            text: control.text
            font: control.font
            color: control.mirrored ? control.palette.brightText : control.palette.buttonText
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        ClippedText {
            clip: control.progress > 0
            clipX: -control.leftPadding
            clipWidth: (control.mirrored ? 1.0 - control.progress : control.progress) * control.width
            visible: control.mirrored ? control.progress < 1 : control.progress > 0

            text: control.text
            font: control.font
            color: control.mirrored ? control.palette.buttonText : control.palette.brightText
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    background: ButtonPanel {
        implicitWidth: 80
        implicitHeight: 24

        control: control
        highlighted: false
        scale: control.mirrored ? -1 : 1

        Rectangle {
            width: control.progress * parent.width
            height: parent.height

            radius: 2
            border.color: Qt.darker(Fusion.highlight(control.palette), 1.4)
            gradient: Gradient {
                GradientStop {
                    position: 0
                    color: Qt.lighter(Fusion.highlight(control.palette), 1.2)
                }
                GradientStop {
                    position: 1
                    color: Fusion.highlight(control.palette)
                }
            }
        }
    }
}
