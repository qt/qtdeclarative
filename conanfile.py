#############################################################################
##
## Copyright (C) 2022 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the release tools of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

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
