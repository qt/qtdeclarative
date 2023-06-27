// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
Item {
    property string greeting: QT_TRANSLATE_NOOP("CustomContext", "hello")

    Text { text: qsTranslate("CustomContext", greeting) }
}
//![0]
