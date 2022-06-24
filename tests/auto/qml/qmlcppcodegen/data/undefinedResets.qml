pragma Strict
import TestTypes

Person {
    name: shoeSize === 11 ? undefined : "Marge"

    onObjectNameChanged: name = undefined
}
