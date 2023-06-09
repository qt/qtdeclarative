import QtQml 2.15

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
