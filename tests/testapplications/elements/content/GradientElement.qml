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

Rectangle {
    id: gradientelement
    property real gradstop: 0.25
    gradient: Gradient {
         GradientStop { position: 0.0; color: "red" }
         GradientStop { position: gradstop; color: "yellow" }
         GradientStop { position: 1.0; color: "green" }
    }
    MouseArea { anchors.fill: parent; enabled: qmlfiletoload == ""; hoverEnabled: true
        onEntered: { helptext = "The gradient should show a trio of colors - red, yellow and green"+
        " - with a slow movement of the yellow up and down the view" }
        onExited: { helptext = "" }
    }
    // Animate the background gradient
    SequentialAnimation { id: gradanim; running: true; loops: Animation.Infinite
        NumberAnimation { target: gradientelement; property: "gradstop"; to: 0.88; duration: 10000; easing.type: Easing.InOutQuad }
        NumberAnimation { target: gradientelement; property: "gradstop"; to: 0.22; duration: 10000; easing.type: Easing.InOutQuad }
    }
}
