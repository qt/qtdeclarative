/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
import "content/elements.js" as Elements
import "content"

Item {
    id: elementsapp; height: 640; width: 360

    property string qmlfiletoload: ""
    property string helptext: ""

    GradientElement { anchors.fill: parent }

    GridViewElement { height: parent.height * .95; width: parent.width * .95; anchors.centerIn: parent; }

    HelpDesk { width: parent.width; height: 200; anchors { bottom: parent.bottom; right: parent.right; bottomMargin: 3; rightMargin: 3 } }

    // Start or remove an .qml when the qmlfiletoload property changes
    onQmlfiletoloadChanged: {
        if (qmlfiletoload == "") {
            Elements.removeApp();
        } else {
            Elements.setapp(qmlfiletoload,elementsapp);
        }
    }

    // Set the qmlfiletoload property with a script function
    function runapp(qmlfile) {
        console.log("Starting ",qmlfile);
        qmlfiletoload = qmlfile;
    }
}
