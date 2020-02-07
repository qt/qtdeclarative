/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick Designer Components.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
