/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import "test.js" as Script

//DO NOT CHANGE

Rectangle {
    id: root
    width: 10; height: 10;
    Component.onCompleted: print("onCompleted")

    property int result:0

    property int someValue: 10

    function doSomething() {
        var a = root.result;
        var b = commonFunction();
        var c = [1,2,3];
        var d = Script.add(a,c[2]);
        result += d;
        doSomethingElse();
    }

    Timer {
        interval: 4000; running: true; repeat: true
        onTriggered: {
            doSomething();
            Script.printMessage("onTriggered");
        }
    }

    function commonFunction() {
        console.log("commonFunction");
        return 5;
    }

    function doSomethingElse() {
        result = Script.add(result,8);
        eval("print(root.result)");
        if (root.result > 15)
            dummy();
    }

}

