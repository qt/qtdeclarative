import QtQml
import test

FrozenObjects {
    id: app;

    property bool caughtSignal: false
    onFooMember2Emitted: caughtSignal = true

    Component.onCompleted: {
        app.fooMember.name = "John";
        app.fooMemberConst;
        app.fooMember.name = "Jane";

        app.fooMember2.name = "John";
        app.triggerSignal();
        app.fooMember2.name = "Jane";
    }
}
