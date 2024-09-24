import QtQml
import Handlerei

HandleHandler {
    property var handleDelegateOutter: handleDelegate
    handle: QtObject { id: handleDelegate }
}
