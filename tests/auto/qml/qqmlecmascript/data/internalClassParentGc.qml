import QtQml

import "internalClassParentGc.js" as Foo

QtObject {
    Component.onCompleted: {
        gc();
        Foo.init();
        Foo.func();
        Foo.nasty();
        objectName = Foo.func()[0];
    }
}
