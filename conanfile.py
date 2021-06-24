#############################################################################
##
## Copyright (C) 2021 The Qt Company Ltd.
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
import os
import re
from functools import lru_cache
from pathlib import Path


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
    "qml-sequence-object",
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
]


@lru_cache(maxsize=8)
def _parse_qt_version_by_key(key: str) -> str:
    with open(Path(Path(__file__).parent.resolve() / ".cmake.conf")) as f:
        ret = [m.group(1) for m in [re.search(r"{0} .*\"(.*)\"".format(key), f.read())] if m]
    return ret.pop() if ret else ""


def _get_qt_minor_version() -> str:
    return _parse_qt_version_by_key('QT_REPO_MODULE_VERSION')[0:3]


class QtDeclarative(ConanFile):
    name = "qtdeclarative"
    license = "LGPL-3.0-only, Commercial Qt License Agreement"
    author = "The Qt Company <https://www.qt.io/contact-us>"
    url = "https://code.qt.io/cgit/qt/qtdeclarative.git/"
    description = (
        "The Qt Declarative module provides a declarative framework for building highly dynamic, "
        "custom user interfaces"
    )
    topics = ("qt", "qt6", "qtdeclarative")
    settings = "os", "compiler", "arch", "build_type"
    exports = ".cmake.conf"  # for referencing the version number and prerelease tag
    exports_sources = "*", "!conan*.*"
    # use commit ID as the RREV (recipe revision) if this is exported from .git repository
    revision_mode = "scm" if Path(Path(__file__).parent.resolve() / ".git").exists() else "hash"
    python_requires = f"qt-conan-common/{_get_qt_minor_version()}@qt/everywhere"

    options = {item.replace("-", "_"): ["yes", "no", None] for item in _qtdeclarative_features}
    default_options = {item.replace("-", "_"): None for item in _qtdeclarative_features}

    def set_version(self):
        _ver = _parse_qt_version_by_key("QT_REPO_MODULE_VERSION")
        _prerelease = _parse_qt_version_by_key("QT_REPO_MODULE_PRERELEASE_VERSION_SEGMENT")
        self.version = _ver + "-" + _prerelease if _prerelease else _ver

    def requirements(self):
        _version = _parse_qt_version_by_key("QT_REPO_MODULE_VERSION")
        # will match latest prerelase of final major.minor.patch
        self.requires(f"qtbase/[<={_version}, include_prerelease=True]@{self.user}/{self.channel}")
        self.requires(f"qtsvg/[<={_version}, include_prerelease=True]@{self.user}/{self.channel}")
        self.requires(f"qtshadertools/[<={_version}, include_prerelease=True]@{self.user}/{self.channel}")

    def build(self):
        self.python_requires["qt-conan-common"].module.build_leaf_qt_module(self)

    def package(self):
        cmd = ["cmake", "--install", "."]
        self.run(" ".join(cmd), run_environment=True)

    def package_info(self):
        self.python_requires["qt-conan-common"].module.package_info(self)

    def package_id(self):
        self.info.requires.package_revision_mode()

    def deploy(self):
        self.copy("*")  # copy from current package
        if not os.environ.get("QT_CONAN_INSTALL_SKIP_DEPS"):
            self.copy_deps("*")  # copy from dependencies
