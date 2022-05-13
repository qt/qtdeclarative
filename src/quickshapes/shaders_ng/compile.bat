:: Copyright (C) 2019 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

:: For HLSL we invoke fxc.exe (-c argument) and store the resulting intermediate format
:: instead of HLSL source, so this needs to be run on Windows from a developer command prompt.

:: For SPIR-V the optimizer is requested (-O argument) which means spirv-opt must be
:: invokable (e.g. because it's in the PATH from the Vulkan SDK)

qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o lineargradient.vert.qsb lineargradient.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o lineargradient.frag.qsb lineargradient.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o radialgradient.vert.qsb radialgradient.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o radialgradient.frag.qsb radialgradient.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o conicalgradient.vert.qsb conicalgradient.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o conicalgradient.frag.qsb conicalgradient.frag
