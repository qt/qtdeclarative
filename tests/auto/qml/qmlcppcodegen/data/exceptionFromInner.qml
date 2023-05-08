pragma Strict
import QtQml

QtObject {
    property QtObject theNull: null

    function doFail() : string { return theNull.objectName }
    function delegateFail() : string { doFail() }
    function disbelieveFail() : string { delegateFail() }
}
