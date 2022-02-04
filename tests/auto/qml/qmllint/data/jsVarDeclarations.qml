import QtQuick

import "jsVarDeclarations.js" as JSVars

Item {
    property int varProp: JSVars.varProp
    property var letProp: JSVars.letProp
    property string constProp: JSVars.constProp
}
