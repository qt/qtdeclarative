// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [document]
import QtQuick

Item {
    width: 200
    height: 250

    ListModel {
        id: myModel
        ListElement { type: "Dog"; age: 8; noise: "meow" }
        ListElement { type: "Cat"; age: 5; noise: "woof" }
    }

    component MyDelegate : Text {
        required property string type
        required property int age
        text: type + ", " + age
        // WRONG: Component.onCompleted: () => console.log(noise)
        // The above line would cause a ReferenceError
        // as there is no required property noise,
        // and the presence of the required properties prevents
        // noise from being injected into the scope
    }

    ListView {
        anchors.fill: parent
        model: myModel
        delegate: MyDelegate {}
    }
}
//! [document]
