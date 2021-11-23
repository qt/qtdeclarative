import QtQml

QtObject {
    id: root
    signal testSignal(string a, int b, string c, bool d, bool e, real f, real g, bool h, int i, int j, string k, int l, string m, string n)
    signal foo()
    onTestSignal: foo()

    property Timer timer: Timer {
        interval: 10
        running: true
        onTriggered: root.testSignal("a", 1, "b", true, true, 0.1, 0.1, true, 1, 1, "a", 1, "a", "a")
    }
}
