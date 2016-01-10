/****************************************************************************
**
** Copyright (C) 2016 basysKom GmbH, opensource@basyskom.com.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
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
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

Item {
    property bool success: false
    property bool complete: false

    property vector2d v2: Qt.vector2d(-2, 0)
    property vector3d v3: Qt.vector3d(-2, 0, 0)
    property vector4d v4: Qt.vector4d(-2, 0, 0, 0)

    property int v2ChangedSignalCount;
    property int v3ChangedSignalCount;
    property int v4ChangedSignalCount;

    onV2Changed: v2ChangedSignalCount++
    onV3Changed: v3ChangedSignalCount++
    onV4Changed: v4ChangedSignalCount++

    Component.onCompleted: {
        complete = false;
        success = true;

        // storing the initial value causes a signal emission
        if (v2ChangedSignalCount !== 1) success = false
        v2 = Qt.vector2d(-2, 0);
        // setting the same value again must not emit a signal
        if (v2ChangedSignalCount !== 1) success = false
        v2.x++
        if (v2ChangedSignalCount !== 2) success = false
        v2.x++  // cycle through 0, 0 which is the default value
        if (v2ChangedSignalCount !== 3) success = false
        v2.x++
        if (v2ChangedSignalCount !== 4) success = false

        // storing the initial value causes a signal emission
        if (v3ChangedSignalCount !== 1) success = false
        v3 = Qt.vector3d(-2, 0, 0);
        // setting the same value again must not emit a signal
        if (v3ChangedSignalCount !== 1) success = false
        v3.x++
        if (v3ChangedSignalCount !== 2) success = false
        v3.x++ // cycle through 0, 0, 0 which is the default value
        if (v3ChangedSignalCount !== 3) success = false
        v3.x++
        if (v3ChangedSignalCount !== 4) success = false

        // storing the initial value causes a signal emission
        if (v4ChangedSignalCount !== 1) success = false
        v4 = Qt.vector4d(-2, 0, 0, 0);
        // setting the same value again must not emit a signal
        if (v4ChangedSignalCount !== 1) success = false
        v4.x++
        if (v4ChangedSignalCount !== 2) success = false
        v4.x++ // cycle through 0, 0, 0 which is the default value
        if (v4ChangedSignalCount !== 3) success = false
        v4.x++
        if (v4ChangedSignalCount !== 4) success = false

        complete = true;
    }
}
