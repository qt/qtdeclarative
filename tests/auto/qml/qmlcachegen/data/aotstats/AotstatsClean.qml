import QtQml

QtObject {
    property int i: 100
    property int j: i * 2

    function s() : string {
        let s = "abc"
        return s + "def "
    }
}
