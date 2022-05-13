// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml 2.15
import QtQuick 2.15

Text {
    id: root
    visible: false

    property bool exceptionAcive: Qt.uiLanguage === root.languageCode


    property string languageCode
    onExceptionAciveChanged: {
        root.__setup()
    }

    Component.onCompleted: root.__setup()

    function __setup() {
        var p = parent
        if (parent.languageExceptionItem !== undefined) {
            if (root.exceptionAcive) {
                parent.languageExceptionItem = root
            } else {
                if (parent.languageExceptionItem === root)
                    parent.languageExceptionItem = parent.__backupText
            }
        }
    }

}

/*##^##
Designer {
    D{i:0;autoSize:true;height:19;width:70}
}
##^##*/
