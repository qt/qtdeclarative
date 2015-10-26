import QtQuick 2.0
import Qt.labs.controls 1.0

//! [1]
SpinBox {
    from: 0
    to: items.length - 1
    value: 1 // "Medium"

    property var items: ["Small", "Medium", "Large"]

    validator: RegExpValidator {
        regExp: new RegExp("(Small|Medium|Large)", "i")
    }

    textFromValue: function(value) {
        return items[value];
    }

    valueFromText: function(text) {
        for (var i = 0; i < items.length; ++i) {
            if (items[i].toLowerCase().indexOf(text.toLowerCase()) === 0)
                return i
        }
        return sb.value
    }
}
//! [1]
