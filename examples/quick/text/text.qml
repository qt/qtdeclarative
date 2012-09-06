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
    \title QML Examples - Text
    \example quick/text
    \brief This is a collection of QML examples
    \image qml-text-example.png

    This is a collection of  small QML examples relating to text. Each example is
    a small QML file, usually containing or emphasizing a particular element or
    feature. You can run and observe the behavior of each example.

    'Hello' shows how to change and animate the letter spacing of a Text element.
    It uses a sequential animation to first animate the font.letterSpacing property
    from 0 to 50 over 3 seconds and then move the text to a random position on screen:
    \snippet examples/quick/text/fonts/hello.qml letterspacing

    'Fonts' shows different ways of using fonts with the Text element.
    Simply by name, using the font.family property directly:
    \snippet examples/quick/text/fonts/fonts.qml name
    or using a FontLoader element:
    \snippet examples/quick/text/fonts/fonts.qml fontloader
    or using a FontLoader and specifying a local font file:
    \snippet examples/quick/text/fonts/fonts.qml fontloaderlocal
    or finally using a FontLoader and specifying a remote font file:
    \snippet examples/quick/text/fonts/fonts.qml fontloaderremote

    'Available Fonts' shows how to use the QML global Qt object and a list view
    to display all the fonts available on the system.
    The ListView element uses the list of fonts available as its model:
    \snippet examples/quick/text/fonts/availableFonts.qml model
    Inside the delegate, the font family is set with the modelData:
    \snippet examples/quick/text/fonts/availableFonts.qml delegate

    'Banner' is a simple example showing how to create a banner using a row of text
    elements and a NumberAnimation.

    'Img tag' shows different ways of displaying images in a text elements using
    the <img> tag.

    'Text Layout' shows how to create a more complex layout for a text element.
    This example lays out the text in two columns using the onLineLaidOut handler
    that allows you to position and resize each line:
    \snippet examples/quick/text/styledtext-layout.qml layout
*/

Item {
    height: 480
    width: 320
    LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Hello", "An Animated Hello World", Qt.resolvedUrl("fonts/hello.qml"));
            addExample("Fonts", "Using various fonts with a Text element", Qt.resolvedUrl("fonts/fonts.qml"));
            addExample("Available Fonts", "A list of your available fonts",  Qt.resolvedUrl("fonts/availableFonts.qml"));
            addExample("Banner", "Large, scrolling text", Qt.resolvedUrl("fonts/banner.qml"));
            addExample("Img tag", "Embedding images into text", Qt.resolvedUrl("imgtag/imgtag.qml"));
            addExample("Text Layout", "Flowing text around items", Qt.resolvedUrl("styledtext-layout.qml"));
        }
    }
}
