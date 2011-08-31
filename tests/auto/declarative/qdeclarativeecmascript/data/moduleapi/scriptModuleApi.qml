import QtQuick 2.0
import Qt.test.scriptApi 1.0 as QtTestScriptApi                 // script module API installed into new uri

QtObject {
    property int scriptTest: QtTestScriptApi.scriptTestProperty // script module api's only provide properties.
}
