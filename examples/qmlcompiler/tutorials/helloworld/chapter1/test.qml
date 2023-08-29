// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
     id: root

     property string greeting: "Hello"

     component MyText : Text {}

     component NotText : Item {
         property string text
     }

     Text { text: "Hello world!" }
     Text { text: root.greeting }
     Text { text: "Goodbye world!" }
     NotText {
         text: "Does not trigger"
          MyText { text: "Goodbye world!" }
     }
}
