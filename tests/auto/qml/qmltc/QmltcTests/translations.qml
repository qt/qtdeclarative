// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Text {
    property string alsoTranslated
    alsoTranslated: qsTr("Bye bye!", "") // has bindable, compiled as translation by qmltc
    text: qsTr("Bye bye!") // has no bindable, compiled as translation by qmltc

    property string hardcodedContext: qsTranslate("translations", "Bye bye!", "") //compiled as String Literal
    property string anotherContext: qsTranslate("anotherContext", "Bye bye!", "") //compiled as String Literal
    property string toBeTranslatedLater: QT_TR_NOOP("Bye bye!") //compiled as String Literal
    property string toBeTranslatedLaterWithHardcodedContext: QT_TRANSLATE_NOOP("translations", "Bye bye!", "", -1) //compiled as Script
    property string translatedN: qsTr("The solution is %n", "", 42) //compiled as translation by qmltc

    property string translatedNWithContextAndAmbiguation: qsTranslate("translations", "The solution has %n degrees", "solution to problem" , 42) //compiled as script
    property string translatedNWithContextAndAmbiguation2: qsTranslate("translations", "The solution has %n degrees", "chemical solution" , 43) //compiled as script

    property string combination: qsTr(qsTr("Bye bye!", ""))
}
