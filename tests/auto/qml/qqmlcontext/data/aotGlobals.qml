pragma Strict
import QtQml

QtObject {
    property var captured

    function capture(): void { captured = parseInt }
    function consoleLog(): void { console.log("hello") }
}
