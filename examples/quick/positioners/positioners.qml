/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import "../../shared" as Examples

/*!
    \title QtQuick Examples - Positioners
    \example quick/positioners
    \brief This is a collection of QML Positioner examples.
    \image qml-positioners-example.png

    This is a collection of small QML examples relating to positioners. Each example is
    a small QML file emphasizing a particular element or feature.

    Transitions shows animated transistions when showing or hiding items in a positioner.
    It consists of a scene populated with items in a variety of positioners: Column, Row, Grid and Flow.
    Each positioner has animations described as Transitions.
    \snippet examples/quick/positioners/positioners-transitions.qml move
    The move transition specifies how items inside the positioner will animate when they are displaced by items appearing or disappearing.
    \snippet examples/quick/positioners/positioners-transitions.qml add
    The add transition specifies how items will appear when they are added to the positioner.

    Attached Properties show using the attached property to determine where in a positioner an item is.
    \snippet examples/quick/positioners/positioners-attachedproperties.qml 0
*/

Item {
    height: 480
    width: 320
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Transitions", "Fluidly shows and hides elements",  Qt.resolvedUrl("positioners-transitions.qml"));
            addExample("Attached Properties", "Knows where it is in the positioner", Qt.resolvedUrl("positioners-attachedproperties.qml"));
        }
    }
}
