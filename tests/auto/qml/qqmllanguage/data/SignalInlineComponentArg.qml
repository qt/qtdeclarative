import QtQuick

Item {
    component Abc: Item {
        property string success
    }

    signal canYouFeelIt(arg1: Abc)
    property Abc someAbc: Abc {
        success: "Signal was called"
    }
    property string success: "Signal not called yet"

    Component.onCompleted: {
        canYouFeelIt(someAbc);
    }

    onCanYouFeelIt: (arg) => {
        success = arg.success
    }
}
