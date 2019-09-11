import QtQml 2.12

import "mylibrary.js" as Lib

QtObject {
    property string german1: Lib.translation_fail()
    property string german2: Lib.translation_success()
}
