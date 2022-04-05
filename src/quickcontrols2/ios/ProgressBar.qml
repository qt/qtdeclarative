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
import QtQuick.Controls.impl
import QtQuick.Controls.iOS

T.ProgressBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    opacity: control.enabled ? 1 : 0.5

    contentItem: Item {
        parent: control.background
        implicitWidth: progress.width
        implicitHeight: progress.implicitHeight
        scale: control.mirrored ? -1 : 1

        readonly property NinePatchImage progress: NinePatchImage {
            parent: control.contentItem
            y: (parent.height - height) / 2
            width: control.indeterminate ? control.width * 0.4 : control.position * parent.width

            source: control.IOS.url + "slider-progress"
            NinePatchImageSelector on source {
                states: [
                    {"light": control.IOS.theme == IOS.Light},
                    {"dark": control.IOS.theme == IOS.Dark}
                ]
            }

            NumberAnimation on x {
                running: control.indeterminate && control.visible
                from: -control.contentItem.progress.width
                to: control.width
                duration: 900
                easing.type: Easing.Linear
                loops: Animation.Infinite
                // TODO: workaround for QTBUG-38932; remove once that is fixed
                onFromChanged: restart()
                onToChanged: restart()
            }
        }
    }

    background: Item {
        implicitWidth: 150
        implicitHeight: children[0].implicitHeight
        clip: control.indeterminate
        NinePatchImage {
            source: control.IOS.url + "slider-background"
            y: (parent.height - height) / 2
            width: control.background.width
            NinePatchImageSelector on source {
                states: [
                    {"light": control.IOS.theme == IOS.Light},
                    {"dark": control.IOS.theme == IOS.Dark}
                ]
            }
        }
    }
}
