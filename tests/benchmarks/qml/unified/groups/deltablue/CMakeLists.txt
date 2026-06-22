# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Main and Strength carry "pragma Singleton"; they must be registered as composite
# singleton types or every reference (Strength.REQUIRED, ...) fails to resolve. The
# property is read by both bundle targets created in this directory by
# unified_add_bundle().
set_source_files_properties(Main.qml Strength.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

unified_add_bundle(deltablue)
