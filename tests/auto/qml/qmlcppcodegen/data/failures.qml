import QtQml
import TestTypes
import TestTypes as TT2
import Ambiguous 1.2

QtObject {
    id: self
    property string attachedForNonObject: objectName.Component.objectName
    property string attachedForNasty: Nasty.objectName

    property Nasty nasty: Nasty {
        objectName: Component.objectName
    }

    onFooBar: console.log("nope")

    function doesReturnValue() { return 5; }

    property Thing thing: Thing {
        property int b: a + 1
    }

    property Thing2 thing2: Thing2 {
        property int b: a + 2
    }

    property NotHere here: NotHere {
        property int c: b + 1
    }

    Component.onCompleted: doesNotExist()

    property string aString: self + "a"

    property BirthdayParty party: BirthdayParty {
        onPartyStarted: (foozle) => { objectName = foozle }
    }

    signal foo()
    signal bar()

    // Cannot assign potential undefined
    onFoo: objectName = self.bar()

    property int enumFromGadget1: GadgetWithEnum.CONNECTED + 1
    property int enumFromGadget2: TT2.GadgetWithEnum.CONNECTED + 1

    function constStore() : int {
        const x = 1;
        x = 2;
        return x;
    }

    function earlyLoad() : int {
        var a = b;
        let b = 5;
        return a;
    }

    function earlyStore() : int {
        a = 5;
        let a;
        return a;
    }
}
