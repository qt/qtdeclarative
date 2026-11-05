pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: root

    property string calls
    property string errors

    function run(caseName: string) {
        try {
            root[caseName]();
        } catch (error) {
            errors += error.message;
        }
    }

    function record(value) { calls += value + ";"; }
    function recordBetween() { calls += "between;"; }

    function scheduleRepeatedly() {
        Qt.callLater(root, record, "once");
        Qt.callLater(root, record, "once");
        Qt.callLater(root, record, "once");
    }

    function rescheduleAroundOtherCall() {
        Qt.callLater(root, record, "repeated");
        Qt.callLater(root, recordBetween);
        Qt.callLater(root, record, "repeated");
    }

    function scheduleWithDifferentArguments() {
        Qt.callLater(root, record, "a");
        Qt.callLater(root, record, "b");
    }

    function scheduleWithDifferentArgumentCounts() {
        Qt.callLater(root, record, "a");
        Qt.callLater(root, record, "a", "extra");
    }

    function recordName(object) { record(object.name); }
    function scheduleWithEqualButDistinctObjects() {
        Qt.callLater(root, recordName, { name: "x" });
        Qt.callLater(root, recordName, { name: "x" });
    }

    QtObject { id: firstObject }
    QtObject { id: secondObject }
    function scheduleForTwoObjects() {
        Qt.callLater(firstObject, record, "same");
        Qt.callLater(secondObject, record, "same");
    }

    function scheduleInBothForms() {
        Qt.callLater(record, "same");
        Qt.callLater(root, record, "same");
    }

    Component {
        id: targetComponent
        QtObject {}
    }
    property QtObject target // reads null once the object is deleted
    function reportTarget() { calls += (target ? "alive;" : "deleted;"); }

    function scheduleThenDestroy() {
        target = targetComponent.createObject(root);
        Qt.callLater(target, record, "dropped");
        target.destroy();
        Qt.callLater(root, reportTarget);
    }

    function scheduleClosureThenDestroy() {
        target = targetComponent.createObject(root);
        Qt.callLater(target, () => record(target.objectName));
        target.destroy();
        Qt.callLater(root, reportTarget);
    }

    property bool guardWithPageChild
    Loader {
        id: loader
        active: false
        sourceComponent: Item {
            id: page
            // Loader deletes only the page, but invalidates the context of the objects inside it
            Item { id: pageChild }
            // record() isn't defined in the page, so only the object can skip the call
            Component.onCompleted: Qt.callLater(root.guardWithPageChild ? pageChild : page,
                                                root.record, "page")
        }
    }
    function loadScheduleUnload() {
        loader.active = true;
        loader.active = false;
    }
    function loadScheduleForPageChildUnload() {
        guardWithPageChild = true;
        loadScheduleUnload();
    }

    Component {
        id: schedulesOwnFunction
        Item {
            id: ownPage
            function work() { root.record(describeWork()); }
            function describeWork() { return "work"; }
            // Without the object, work() runs and fails to look up describeWork()
            Component.onCompleted: Qt.callLater(ownPage, work)
        }
    }
    function loadScheduleOwnFunctionUnload() {
        loader.sourceComponent = schedulesOwnFunction;
        loadScheduleUnload();
    }

    ListModel {
        id: model
        onCountChanged: root.calls += "count=" + count + ";"
    }
    function scheduleCppMethod() {
        const row = { name: "row" };
        Qt.callLater(model, model.append, row);
        Qt.callLater(root, recordBetween);
        Qt.callLater(model, model.append, row);
        Qt.callLater(root, reportModel);
    }
    function reportModel() { calls += model.get(0).name + ";"; }

    function passNonFunction() { Qt.callLater(root, 42); }
    function passNullObject() { Qt.callLater(null, record); }
    function passUndefinedObject() { Qt.callLater(undefined, record); }
}
