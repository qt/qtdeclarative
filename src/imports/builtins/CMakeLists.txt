# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(qml_type_files builtins.qmltypes jsroot.qmltypes)

qt_path_join(qml_install_dir "${QT_INSTALL_DIR}" "${INSTALL_QMLDIR}")
qt_copy_or_install(FILES ${qml_type_files}
    DESTINATION ${qml_install_dir}
)

# in prefix builds we also need to copy the files into the build directory of
# the module, so that they are located together with the QML modules
if(QT_WILL_INSTALL)
    qt_path_join(qml_build_dir "${QT_BUILD_DIR}" "${INSTALL_QMLDIR}")
    file(COPY ${qml_type_files} DESTINATION ${qml_build_dir})
endif()
