# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from conans import ConanFile
import re
from pathlib import Path
from typing import Dict, Any

_qtdeclarative_features = [
    "qml-animation",
    "qml-debug",
    "qml-delegate-model",
    "qml-devtools",
    "qml-itemmodel",
    "qml-jit",
    "qml-list-model",
    "qml-locale",
    "qml-network",
    "qml-object-model",
    "qml-preview",
    "qml-profiler",
    "qml-table-model",
    "qml-worker-script",
    "qml-xml-http-request",
    "qml-xmllistmodel",
    "quick-animatedimage",
    "quick-canvas",
    "quick-designer",
    "quick-draganddrop",
    "quick-flipable",
    "quick-gridview",
    "quick-listview",
    "quick-particles",
    "quick-path",
    "quick-pathview",
    "quick-positioners",
    "quick-repeater",
    "quick-shadereffect",
    "quick-sprite",
    "quick-tableview",
    "quick-treeview",
    "quickcontrols2-fusion",
    "quickcontrols2-imagine",
    "quickcontrols2-macos",
    "quickcontrols2-material",
    "quickcontrols2-universal",
    "quickcontrols2-windows",
    "quicktemplates2-calendar",
    "quicktemplates2-hover",
    "quicktemplates2-multitouch",
]


def _parse_qt_version_by_key(key: str) -> str:
    with open(Path(__file__).parent.resolve() / ".cmake.conf") as f:
        m = re.search(fr'{key} .*"(.*)"', f.read())
    return m.group(1) if m else ""


def _get_qt_minor_version() -> str:
    return ".".join(_parse_qt_version_by_key("QT_REPO_MODULE_VERSION").split(".")[:2])


class QtDeclarative(ConanFile):
    name = "qtdeclarative"
    license = "LGPL-3.0, GPL-2.0+, Commercial Qt License Agreement"
    author = "The Qt Company <https://www.qt.io/contact-us>"
    url = "https://code.qt.io/cgit/qt/qtdeclarative.git"
    description = (
        "The Qt Declarative module provides a declarative framework for building highly dynamic, "
        "custom user interfaces"
    )
    topics = "qt", "qt6", "qtdeclarative"
    settings = "os", "compiler", "arch", "build_type"
    # for referencing the version number and prerelease tag and dependencies info
    exports = ".cmake.conf", "dependencies.yaml"
    exports_sources = "*", "!conan*.*"
    python_requires = f"qt-conan-common/{_get_qt_minor_version()}@qt/everywhere"
    python_requires_extend = "qt-conan-common.QtLeafModule"

    def get_qt_leaf_module_options(self) -> Dict[str, Any]:
        """Implements abstractmethod from qt-conan-common.QtLeafModule"""
        return self._shared.convert_qt_features_to_conan_options(_qtdeclarative_features)

    def get_qt_leaf_module_default_options(self) -> Dict[str, Any]:
        """Implements abstractmethod from qt-conan-common.QtLeafModule"""
        return self._shared.convert_qt_features_to_default_conan_options(_qtdeclarative_features)
