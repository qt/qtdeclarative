/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtQml 2.14

Rectangle {
    height: 400
    width: 400

    Rectangle {
        property bool when: true

        id: myItem
        objectName:  "myItem"
        height: 300
        width: 200
        property var foo: original
        property bool fooCheck: foo === original && foo.bar() === 42
    }

    property var original: ({ bar: function() { return 42 } })

    property var boundValue: 13

    Binding {
        restoreMode: Binding.RestoreBinding
        objectName: "theBinding"
        target: myItem
        property: "foo"
        when: myItem.when
        value: boundValue
    }
}
