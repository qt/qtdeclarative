import QtQuick

ScriptAction {
    property int before: 1 + 1
    script: console.log("script")
    property int after: 1 + 1
}
