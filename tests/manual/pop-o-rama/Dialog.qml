/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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
import QtQuick.Window 2.2
import Qt.labs.templates 1.0 as T
import Qt.labs.controls 1.0

T.Panel {
    id: root

    default property alias contentData: dialogFrame.contentData

    focus: true
    modal: true

    contentItem: Frame {
        id: dialogFrame

        padding: 6
        implicitWidth: leftPadding + contentWidth + rightPadding
        implicitHeight: topPadding + contentHeight + bottomPadding
        contentWidth: contentItem.__singleChild ? contentItem.children[0].width : 0
        contentHeight: contentItem.__singleChild ? contentItem.children[0].height : 0

        focus: true
        opacity: 0
        x: (Window.width - width) / 2
        y: (Window.height - height) / 2

        contentItem: FocusScope {
            id: ct
            readonly property bool __singleChild: children.length === 1
            scale: 0.1
            focus: true
            x: dialogFrame.leftPadding
            y: dialogFrame.topPadding
        }

        frame: Rectangle {
            id: fr
            width: dialogFrame.width
            height: dialogFrame.height
            scale: 0.1
            color: Theme.backgroundColor
            border.color: Theme.frameColor
            border.width: 1
            radius: 3
        }

        background: Rectangle {
            id: bg
            x: -dialogFrame.x
            y: -dialogFrame.y
            width: Window.width
            height: Window.height
            color: "black"
            opacity: 0.3
        }
    }

    showTransition: Transition {
        ScaleAnimator {
            target: fr
            duration: 100
            to: 1.0
        }

        ScaleAnimator {
            target: ct
            duration: 100
            to: 1.0
        }

        OpacityAnimator {
            target: dialogFrame
            duration: 100
            to: 1.0
        }
    }

    hideTransition: Transition {
        ScaleAnimator {
            target: fr
            duration: 100
            from: 1.0; to: 0.1
        }

        ScaleAnimator {
            target: ct
            duration: 100
            from: 1.0; to: 0.1
        }

        OpacityAnimator {
            target: dialogFrame
            duration: 200
            from: 1.0; to: 0
        }
    }
}
