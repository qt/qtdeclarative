/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.6
import Qt.labs.templates 1.0 as T
import Qt.labs.controls.universal 1.0
import Qt.labs.controls.universal.impl 1.0

T.ProgressBar {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            indicator ? indicator.implicitWidth : 0) + leftPadding + rightPadding
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                             indicator ? indicator.implicitHeight : 0) + topPadding + bottomPadding

    //! [indicator]
    indicator: Rectangle {
        x: control.leftPadding
        y: control.topPadding
        width: control.indeterminate ? 0 : control.position * control.availableWidth
        height: control.availableHeight

        scale: control.mirrored ? -1 : 1
        color: control.Universal.accentColor

        ProgressStrip {
            id: strip

            width: control.availableWidth
            height: control.availableHeight

            clip: control.indeterminate
            visible: control.indeterminate
            color: control.Universal.accentColor

            ProgressStripAnimator {
                target: strip
                running: strip.visible
            }
        }
    }
    //! [indicator]

    //! [background]
    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 10

        x: control.leftPadding
        y: control.topPadding
        width: control.availableWidth
        height: control.availableHeight

        visible: !control.indeterminate
        color: control.Universal.baseLowColor
    }
    //! [background]
}
