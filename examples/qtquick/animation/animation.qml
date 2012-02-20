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
    \title QtQuick Examples - Animation
    \example qtquick/animation
    \brief This is a collection of QML Animation examples.
    \image qml-animations-example.png

    This is a collection of small QML examples relating to animation. Each example is
    a small QML file emphasizing a particular element or feature.

    ColorAnimation demonstrates using a color animation to fade a sky from day to night.

    PropertyAnimation demonstrates using a number animation to bounce a circle up and down.

    Behaviors demonstrates using behaviors to animate moving a rectangle to whereever you click.

    Wiggly Text demonstrates using more complex behaviors to animate and wiggle some text around as you drag it.

    Easing Curves shows off all the easing curves available in Qt Quick animations.

    States demonstrates how the properties of an item can vary between states.

    Transitions takes the States example and animates the property changes.

    PathAnimation animates an image along a beizer curve using a PathAnimation.

    PathInterpolator animates an image along the same beizer curve, using a PathInterpolator instead.
*/

Item {
    height: 480
    width: 320
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("ColorAnimation", "Interpolates between colors",  Qt.resolvedUrl("basics/color-animation.qml"));
            addExample("PropertyAnimation", "Interpolates between numbers", Qt.resolvedUrl("basics/property-animation.qml"));
            addExample("Behaviors", "Animates procedural movement", Qt.resolvedUrl("behaviors/behavior-example.qml"));
            addExample("Wiggly Text", "Text that wiggles as you drag it", Qt.resolvedUrl("behaviors/wigglytext.qml"));
            addExample("Easing Curves", "Compare available easing curves", Qt.resolvedUrl("easing/easing.qml"));
            addExample("States", "Simple states", Qt.resolvedUrl("states/states.qml"));
            addExample("Transitions", "Simple states with animated transitions", Qt.resolvedUrl("states/transitions.qml"));
            addExample("PathAnimation", "Animate along a path", Qt.resolvedUrl("pathanimation/pathanimation.qml"));
            addExample("PathInterpolator", "Interpolates along a path", Qt.resolvedUrl("pathinterpolator/pathinterpolator.qml"));
        }
    }
}
