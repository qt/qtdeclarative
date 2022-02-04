import QtQuick

import "jsVarDeclarations.js" as JSVars

Item {
    Component.onCompleted: { JSVars.constProp = "new value"; }
}
