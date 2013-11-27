/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.2

/*
   The purpose of the test is to verify that a batch of more than 64K
   vertices gets split into multiple drawsets and still rendered correctly.
   Both the clipped and unclipped batches have 50.000 rectangles resulting
   in 200.000 vertices in each batch, which should be plenty..

   #samples: 8

                PixelPos     R    G    B    Error-tolerance
   #base:         0   0     1.0  0.0  0.0        0.0
   #base:        99 199     1.0  0.0  0.0        0.0
   #base:       100   0     0.0  0.0  0.0        0.0
   #base:       199 199     0.0  0.0  0.0        0.0
   #final:        0   0     0.0  0.0  1.0        0.0
   #final:       99 199     0.0  0.0  1.0        0.0
   #final:      100   0     0.0  1.0  0.0        0.0
   #final:      199 199     0.0  1.0  0.0        0.0
*/

RenderTestBase
{
    id: root

    Grid {
        id: clipped
        width: 100
        height: 500
        clip: true
        columns: 100
        Repeater {
            id: clippedRepeater
            model: clipped.width * clipped.height
            Rectangle {
                width: 1
                height: 1
                color: index < clippedRepeater.model / 2 ? "red" : "blue";
            }
        }
    }

    Grid {
        id: unclipped
        x: 100
        width: 100
        height: 500
        clip: true
        columns: 100
        Repeater {
            id: unclippedRepeater
            model: unclipped.width * unclipped.height
            Rectangle {
                width: 1
                height: 1
                color: index < unclippedRepeater.model / 2 ? "black" : "#00ff00";
            }
        }
    }

    SequentialAnimation {
        id: animation
        NumberAnimation { target: clipped; property: "y"; from: 0; to: -clipped.height + root.height; duration: 100 }
        NumberAnimation { target: unclipped; property: "y"; from: 0; to: -unclipped.height + root.height; duration: 100 }
        PropertyAction { target: root; property: "finalStageComplete"; value: true; }
    }

    onEnterFinalStage: {
        animation.running = true;

    }
}
