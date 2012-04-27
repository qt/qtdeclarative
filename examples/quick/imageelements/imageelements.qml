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
    \title QML Examples - Image Elements
    \example quick/imageelements
    \brief This is a collection of QML examples
    \image qml-imageelements-example.png

    This is a collection of small QML examples relating to image elements.

    'BorderImage' shows off the various scaling modes of the BorderImage item
    by setting its horizontalTileMode and verticalTileMode properties.

    'Image' shows off the various fill modes of the Image item.

    'Shadows' shows how to create a drop shadow effect for a rectangular item
    using a BorderImage:
    \snippet examples/quick/imageelements/content/ShadowRectangle.qml delegate

    'AnimatedSprite' shows how to display a simple animation using an
    AnimatedSprite element:
    \snippet examples/quick/imageelements/animatedsprite.qml sprite
    The sprite animation will loop 3 times.

    'SpriteSequence' demonstrates using a sprite sequence to draw an animated
    and interactive bear.
    The SpriteSequence defines 5 different sprites. The bear is initially in
    a 'still' state:
    \snippet examples/quick/imageelements/spritesequence.qml still
    When the scene is clicked, an animation sets the sprite sequence to the
    'falling' states and animates the bear's y property.
    \snippet examples/quick/imageelements/spritesequence.qml animation
    At the end of the animation the bear is set back to its initial state.
*/

Item {
    height: 480
    width: 320
    LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("BorderImage", "An image with scaled borders",  Qt.resolvedUrl("borderimage.qml"));
            addExample("Image", "A showcase of the options available to Image", Qt.resolvedUrl("image.qml"));
            addExample("Shadows", "Rectangles with a drop-shadow effect", Qt.resolvedUrl("shadows.qml"));
            addExample("AnimatedSprite", "A simple sprite-based animation", Qt.resolvedUrl("animatedsprite.qml"));
            addExample("SpriteSequence", "A sprite-based animation with complex transitions", Qt.resolvedUrl("spritesequence.qml"));
        }
    }
}
