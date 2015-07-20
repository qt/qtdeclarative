/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
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

import QtQuick 2.6

Timer {
    interval: 500
    running: true
    repeat: true

    property int i: 0
    readonly property var messages: [
        "Wi nøt trei a høliday in Sweden this yër ?",
        "See the løveli lakes",
        "The wøndërful telephøne system",
        "And mäni interesting furry animals",
        "Including the majestik møøse",
        "A Møøse once bit my sister...",
        "No realli! She was Karving her initials øn the møøse with the sharpened end of an interspace tøøthbrush given her by Svenge — her brother-in-law — an Oslo dentist and star of many Norwegian møvies: \"The Høt Hands of an Oslo Dentist\", \"Fillings of Passion\", \"The Huge Mølars of Horst Nordfink\"...  ",
        "Mynd you, møøse bites Kan be pretty nasti..."
    ]

    signal messageReceived(string sender, string message)

    onTriggered: {
        messageReceived("Swedish Tourism Office", messages[i])
        if (i === messages.length - 1)
            i = 0
        else
            i++
        interval = 3000 + 4000 * Math.random()
    }
}
