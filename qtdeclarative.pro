CONFIG += tests_need_tools
load(qt_parts)

ios {
    log("The qtdeclarative module was disabled from the build because the dependency qtjsbackend/V8 is not ported to iOS.")
    SUBDIRS=
}
