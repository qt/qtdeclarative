/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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
import QtQuick.Templates as T
import QtQuick.Controls.iOS
import QtQuick.Controls.impl

T.Slider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitHandleHeight + topPadding + bottomPadding)

    handle: Item {
        implicitWidth: children[0].implicitWidth - children[0].leftInset - children[0].rightInset
        implicitHeight: children[0].implicitWidth - children[0].topInset - children[0].bottomInset
        x: Math.round(control.leftPadding + (control.horizontal ? control.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2))
        y: Math.round(control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.visualPosition * (control.availableHeight - height)))

        NinePatchImage {
            x: -leftInset
            y: -topInset
            source: control.IOS.url + "slider-handle"
            NinePatchImageSelector on source {
                states: [
                    {"light": IOS.theme == IOS.Light},
                    {"dark": IOS.theme == IOS.Dark},
                ]
            }
        }
    }

    background: Item {
        implicitWidth: control.horizontal ? 114 : children[0].implicitHeight
        implicitHeight: control.horizontal ? children[0].implicitHeight : 114
        opacity: control.enabled ? 1 : 0.5

        NinePatchImage {
            source: control.IOS.url + "slider-background"
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            rotation: control.horizontal ? 0 : -90
            width: control.horizontal ? background.width : background.height
            NinePatchImageSelector on source {
                states: [
                    {"light": IOS.theme == IOS.Light},
                    {"dark": IOS.theme == IOS.Dark},
                ]
            }

            NinePatchImage {
                width: control.handle.width / 2 + control.position * (parent.width - control.handle.width)
                height: parent.height

                source: control.IOS.url + "slider-progress"
                NinePatchImageSelector on source {
                    states: [
                        {"light": IOS.theme == IOS.Light},
                        {"dark": IOS.theme == IOS.Dark},
                    ]
                }
            }
        }
    }
}
