TEMPLATE = aux

QMLTYPEFILE = builtins.qmltypes

# install rule
builtins.files = $$QMLTYPEFILE
builtins.path = $$[QT_INSTALL_QML]
INSTALLS += builtins

# copy to build directory
!prefix_build: COPIES += builtins
