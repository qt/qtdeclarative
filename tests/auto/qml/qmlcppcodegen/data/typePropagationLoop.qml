import QtQml

QtObject {
    property int j:  {
        var tmp = Qt.PartiallyChecked
        for (var i = 0; i < Qt.Checked; i++) {}
        return tmp + i;
    }
}
