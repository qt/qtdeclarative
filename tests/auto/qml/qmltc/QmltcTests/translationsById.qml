// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick

// note: mixing translations with Ids with translations with texts is unsupported. Therefore this is separated from the translations.qml file.

Text {
    property string alsoTranslated
    alsoTranslated: qsTrId("ID1") // has bindable, compiled as translation by qmltc
    text: qsTrId("ID1") // has no bindable, compiled as translation by qmltc

    property string toBeTranslatedLater: QT_TRID_NOOP("ID1") //compiled as String Literal by qmltc!

    property string translatedN: qsTrId("ID2", 5) // compiled as translation by qmltc
}
