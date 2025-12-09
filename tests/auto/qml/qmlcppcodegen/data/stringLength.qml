pragma Strict
import QtQml

QtObject {
    objectName: "astringb"
    property int stringLength: objectName.length
    property string a: {
        const value = objectName;
        if (value && value.length > 0) {
            return value;
        }
        return "no"
    }
}
