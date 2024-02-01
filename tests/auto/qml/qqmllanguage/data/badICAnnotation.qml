import QtQml

QtObject {
    id: self

    function doStuff(status: Binding.NotAnInlineComponent) : int {
        return status
    }

    function doStuff2(status: InlineComponentBase.IC) : QtObject {
        return status
    }

    function doStuff3(status: InlineComponentBase.NotIC) : QtObject {
        return status
    }

    property InlineComponentBase.IC ic: InlineComponentBase.IC {}

    property int a: doStuff(5)
    property QtObject b: doStuff2(ic)
    property QtObject c: doStuff3(ic)
    property QtObject d: doStuff2(self)
}

