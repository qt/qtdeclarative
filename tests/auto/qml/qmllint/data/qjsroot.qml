import QtQuick

QtObject {
    property var locale: Qt.locale()
    property date currentDate: new Date()
    property RegularExpressionValidator validator: RegularExpressionValidator { regularExpression:  /20([0-9]+)/ };
    property double pi: 3.14

    // Test date
    property string dateString: currentDate.toLocaleDateString();

    // Test string
    property int dateStringDotIndex: dateString.indexOf(".")

    // Test regularExpression
    property var dateStringReplace: validator.regularExpression.exec(dateString)

    // Test double
    property string fixedPi: pi.toFixed(1)
}
