# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt6_add_shaders(${test_target} "${test_target}_shaders"
    BATCHABLE
    PREFIX
        "/"
    BASE
        "../"
    FILES
        "../data/combobox/shader.frag"
)
