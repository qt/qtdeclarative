equals(QMAKE_HOST.os, Windows): EXE_SUFFIX = .exe

defineTest(qtConfTest_detectPython) {
    PYTHON_NAMES = python$$EXE_SUFFIX python2$${EXE_SUFFIX} python3$${EXE_SUFFIX} py$${EXE_SUFFIX}
    for (name, PYTHON_NAMES) {
        python_path = $$qtConfFindInPath("$$name")
        !isEmpty(python_path): \
            break()
    }
    isEmpty(python_path) {
        qtLog("No $$PYTHON_NAMES are found in PATH. Giving up.")
        return(false)
    }

    # Make tests.python.location available in configure.json.
    $${1}.location = $$shell_path($$python_path)
    export($${1}.location)
    $${1}.cache += location
    export($${1}.cache)

    return(true)
}
