import QtQml

QtObject {
    property var loadCustom: function(data: int): void  { objectName = "data" + data; }
    Component.onCompleted: loadCustom(12)
}
