import QtQuick

// this file performs two tests: first, using a signal with a inline component from another file
// and second, calling the signal from another file using an inline component from another file

Item {
    signal canYouFeelIt(arg1:SignalInlineComponentArg.Abc)

    property SignalInlineComponentArg.Abc someAbc: SignalInlineComponentArg.Abc {
        success: "Own signal was called with component from another file"
    }

    property SignalInlineComponentArg fromAnotherFile: SignalInlineComponentArg {}

    // success of own signal call with parameter from another file
    property string successFromOwnSignal: "Signal not called yet"
    // makes it easier to test
    property string successFromSignalFromFile: fromAnotherFile.success

    Component.onCompleted: {
        canYouFeelIt(someAbc);
        fromAnotherFile.someAbc.success = "Signal was called from another file"
        fromAnotherFile.canYouFeelIt(fromAnotherFile.someAbc)
    }

    onCanYouFeelIt: (arg) => {
        successFromOwnSignal = arg.success
    }
}

