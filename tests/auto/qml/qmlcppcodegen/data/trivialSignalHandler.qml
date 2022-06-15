pragma Strict
import QtQml

QtObject {
    property string a: "no"
    property int b: -1
    property real c: -1

    signal thingHappened(thing: int)
    signal otherThingHappened(otherThing: real)

    onObjectNameChanged: function() { a = objectName }
    onAChanged: () => { thingHappened(5) }
    onThingHappened: (thing) => { b = thing }
    onBChanged: function() { otherThingHappened(b) }
    onOtherThingHappened: function(otherThing) { c = otherThing / 2 }
}
