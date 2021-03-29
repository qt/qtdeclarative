import QtQml

QtObject {
    @Deprecated {}
    property int deprecated: 500
    @Deprecated { reason: "Test" }
    property int deprecatedReason: 200
}
