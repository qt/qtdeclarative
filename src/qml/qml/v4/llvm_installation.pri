LLVM_CONFIG=llvm-config
# Pick up the qmake variable or environment variable for LLVM_INSTALL_DIR. If either was set, change the LLVM_CONFIG to use that.
isEmpty(LLVM_INSTALL_DIR):LLVM_INSTALL_DIR=$$(LLVM_INSTALL_DIR)
!isEmpty(LLVM_INSTALL_DIR):LLVM_CONFIG=$$LLVM_INSTALL_DIR/bin/llvm-config
exists ($${LLVM_CONFIG}) {
    CONFIG += llvm-libs
    message("Found LLVM in $$LLVM_INSTALL_DIR")
}

llvm-libs {
    win32 {
        LLVM_INCLUDEPATH = $$LLVM_INSTALL_DIR/include
# TODO: check if the next line is needed somehow for the llvm_runtime target.
        LLVM_LIBS += -ladvapi32 -lshell32
    }

    unix {
        LLVM_INCLUDEPATH = $$system($$LLVM_CONFIG --includedir)
        LLVM_LIBDIR = $$system($$LLVM_CONFIG --libdir)
    }

    LLVM_DEFINES += __STDC_LIMIT_MACROS __STDC_CONSTANT_MACROS
}
