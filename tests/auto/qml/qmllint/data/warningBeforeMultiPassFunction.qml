import QtQml

QtObject {
    id: root

    function warns() { console.log(root.doesNotExist) }

    // The type propagator needs more than one pass for this function, which
    // makes it roll back the logger transaction.
    function needsMorePasses() {
        let x = 0
        for (let i = 0; i !== 3; ++i)
            x = root
        return x
    }
}
