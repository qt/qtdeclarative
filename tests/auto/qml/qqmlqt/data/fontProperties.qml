import QtQuick 2.0

Item {
    property font fontProperty: Qt.font({ contextFontMerging: true,
                                          variableAxes: { "abcd": 23.0625 },
                                          features: { "abcd": 23 } });
}
