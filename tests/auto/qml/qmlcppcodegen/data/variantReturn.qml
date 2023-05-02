pragma Strict
import QtQml
import TestTypes

QtObject {
    property DirectBindable a: DirectBindable {
        id: aId
        x: WeatherModelUrlUtils.url(1)
    }

    property IndirectBindable b: IndirectBindable {
        id: bId
        y: aId.x
    }
}
