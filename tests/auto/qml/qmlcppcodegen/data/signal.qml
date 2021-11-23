import QtQml

QtObject {
    property int ff: 4
    onObjectNameChanged: ff = 12
}
