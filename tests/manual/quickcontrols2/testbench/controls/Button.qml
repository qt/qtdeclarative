/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
        ["disabled"],
        ["pressed"],
        ["checkable", "checked"],
        ["checkable", "checked", "disabled"],
        ["checkable", "checked"],
        ["highlighted"],
        ["highlighted", "disabled"],
        ["highlighted", "pressed"],
        ["highlighted", "checkable"],
        ["highlighted", "checkable", "pressed"],
        ["highlighted", "checkable", "checked"],
        ["icon"],
        ["icon", "disabled"],
        ["icon", "pressed"],
        ["icon", "checkable", "checked"],
        ["icon", "checkable", "checked", "disabled"],
        ["icon", "checkable", "checked"],
        ["icon", "highlighted"],
        ["icon", "highlighted", "disabled"],
        ["icon", "highlighted", "pressed"],
        ["icon", "highlighted", "checkable"],
        ["icon", "highlighted", "checkable", "pressed"],
        ["icon", "highlighted", "checkable", "checked"],
        ["flat"],
        ["flat", "disabled"],
        ["flat", "pressed"],
        ["flat", "checkable"],
        ["flat", "checkable", "checked"],
        ["flat", "checkable", "pressed"],
        ["flat", "checkable", "checked", "pressed"],
        ["flat", "checkable", "highlighted"],
        ["flat", "checkable", "highlighted", "pressed"],
        ["flat", "checkable", "highlighted", "checked"],
        ["icon", "flat"],
        ["icon", "flat", "disabled"],
        ["icon", "flat", "pressed"],
        ["icon", "flat", "checkable"],
        ["icon", "flat", "checkable", "checked"],
        ["icon", "flat", "checkable", "pressed"],
        ["icon", "flat", "checkable", "checked", "pressed"],
        ["icon", "flat", "checkable", "highlighted"],
        ["icon", "flat", "checkable", "highlighted", "pressed"],
        ["icon", "flat", "checkable", "highlighted", "checked"]
    ]

    property Component component: Button {
        text: "Button"
        enabled: !is("disabled")
        flat: is("flat")
        checkable: is("checkable")
        checked: is("checked")
        // Only set it if it's pressed, or the non-pressed examples will have no press effects
        down: is("pressed") ? true : undefined
        highlighted: is("highlighted")
        icon.source: is("icon") ? "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png" : ""
    }
}
