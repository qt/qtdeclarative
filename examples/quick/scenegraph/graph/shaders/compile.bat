:: Copyright (C) 2019 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -o noisy.vert.qsb noisy.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o noisy.frag.qsb noisy.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -o line.vert.qsb line.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o line.frag.qsb line.frag
