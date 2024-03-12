// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QmltcTests

Item {

    property var myComponentList
    property list<int> myValueTypeList
    property list<QtObject> myQtObjectList
    property list<HelloWorld> myHelloWorldList

    property Item myItem: Item {
        // test qquickitems default data list (appending to private property of qquickitem)
        HelloWorld {
            hello: "hello1"
        }
        HelloWorld {
            hello: "hello2"
        }
        Rectangle {
            property string hello: "I am a Rectangle."
        }
    }

    myComponentList: [
        "Hello",
        42,
        4.0,
    ]

    myValueTypeList: [
        12489,
        10,
        42
    ]

    myQtObjectList: [
        HelloWorld {
            hello: "Guten Morgen!"
        },
        Rectangle {
            property string hello: "I am a Rectangle."
        },
        HelloWorld {
            hello: "Moin!"
        }
    ]

    myHelloWorldList: [
        HelloWorld{
            hello: "Good morning1"
        },
        HelloWorld {
            hello: "Good morning2"
        },
        HelloWorld {
            hello: "Good morning3"
        }
    ]

    property var extended: TypeWithExtension {
        myList: [
            HelloWorld {},
            Rectangle {},
            QtObject {}
        ]
    }
}
