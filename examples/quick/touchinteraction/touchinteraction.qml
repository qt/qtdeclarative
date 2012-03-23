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
import "../../shared"

/*!
    \title QtQuick Examples - Touch Interaction
    \example qtquick/touchinteraction
    \brief This is a collection of QML Touch Interaction examples.
    \image qml-touchinteraction-example.png

    This is a collection of small QML examples relating to touch interaction methods.

    Multipoint Flames demonstrates distinguishing different fingers in a MultiPointTouchArea, by assignning a different colored flame to each touch point.

    Bear-Whack demonstrates using a MultiPointTouchArea to add multiple finger support to a simple game.

    Flick Resize uses a PinchArea to allow Pinch-to-Resize behavior.

    Flickable is a simple example demonstrating the Flickable element.

    Corkboards shows a more complex Flickable usecase, with elements on the flickable that respond to mouse and keyboard interaction.
*/

Item {
    height: 480
    width: 320
    LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Multipoint Flames", "Create multiple flames with multiple fingers", Qt.resolvedUrl("multipointtouch/multiflame.qml"));
            addExample("Bear-Whack", "Use multiple touches to knock all the bears down",  Qt.resolvedUrl("multipointtouch/bearwhack.qml"));
            addExample("Flick Resize", "Manipulate images using pinch gestures", Qt.resolvedUrl("pincharea/flickresize.qml"));
            addExample("Flickable", "A viewport you can move with touch gestures", Qt.resolvedUrl("flickable/basic-flickable.qml"));
            addExample("Corkboards", "Uses touch input on items inside a Flickable", Qt.resolvedUrl("flickable/corkboards.qml"));
        }
    }
}
