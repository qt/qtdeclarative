# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qmlscenegrabber Binary:
#####################################################################

qt_internal_add_manual_test(qmlscenegrabber
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.." # special case
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::QuickControls2
)

qt6_add_shaders(qmlscenegrabber "qmlscenegrabber_shaders"
    BATCHABLE
    PREFIX
        "/"
    BASE
        "../data/shared"
    FILES
        "../data/shared/shaders/basic_alpha.frag"
        "../data/shared/shaders/basic.frag"
        "../data/shared/shaders/cmyk.frag"
        "../data/shared/shaders/culling.frag"
        "../data/shared/shaders/desaturate.frag"
        "../data/shared/shaders/edge.frag"
        "../data/shared/shaders/gradient2.frag"
        "../data/shared/shaders/gradient3.frag"
        "../data/shared/shaders/gradient4.frag"
        "../data/shared/shaders/gradient5.frag"
        "../data/shared/shaders/gradient.frag"
        "../data/shared/shaders/gradientgrid.frag"
        "../data/shared/shaders/property.frag"
        "../data/shared/shaders/shadow.frag"
        "../data/shared/shaders/stencil.frag"
        "../data/shared/shaders/subrect.vert"
        "../data/shared/shaders/wave.vert"
)
