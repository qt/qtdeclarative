// Copyright (C) 2020 The Qt Company Ltd.

import QtQuick 2.15

Item {
     property bool foo: false

     property alias bar: item.visible

     Item {
         id: item
         visible: parent.foo
     }
}
