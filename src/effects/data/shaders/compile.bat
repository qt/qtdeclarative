:: Copyright (C) 2022 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

:: Multieffect fragment shaders ::
:: c = Common color effects
:: m = Mask
:: b = Blur
:: s = Shadow
:: [n] = Amount of blur items used

qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_c0.frag.qsb multieffect.frag
qsb -DMASK --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cm0.frag.qsb multieffect.frag

:: Special shaders for non-blurred shadows
qsb -DBL0 -DSHADOW --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cs0.frag.qsb multieffect.frag
qsb -DBL0 -DMASK -DSHADOW --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cms0.frag.qsb multieffect.frag

:: Shaders for different blur levels
qsb -DBL1 -DBLUR --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cb1.frag.qsb multieffect.frag
qsb -DBL1 -DSHADOW --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cs1.frag.qsb multieffect.frag
qsb -DBL1 -DMASK -DBLUR --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cmb1.frag.qsb multieffect.frag
qsb -DBL1 -DMASK -DSHADOW --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cms1.frag.qsb multieffect.frag
qsb -DBL1 -DBLUR -DSHADOW --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cbs1.frag.qsb multieffect.frag
qsb -DBL1 -DMASK -DBLUR -DSHADOW --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cmbs1.frag.qsb multieffect.frag

qsb -DBL1 -DBL2 -DBLUR --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cb2.frag.qsb multieffect.frag
qsb -DBL1 -DBL2 -DSHADOW --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cs2.frag.qsb multieffect.frag
qsb -DBL1 -DBL2 -DMASK -DBLUR --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cmb2.frag.qsb multieffect.frag
qsb -DBL1 -DBL2 -DMASK -DSHADOW --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cms2.frag.qsb multieffect.frag
qsb -DBL1 -DBL2 -DBLUR -DSHADOW --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cbs2.frag.qsb multieffect.frag
qsb -DBL1 -DBL2 -DMASK -DBLUR -DSHADOW --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cmbs2.frag.qsb multieffect.frag

qsb -DBL1 -DBL2 -DBL3 -DBLUR --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cb3.frag.qsb multieffect.frag
qsb -DBL1 -DBL2 -DBL3 -DSHADOW --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cs3.frag.qsb multieffect.frag
qsb -DBL1 -DBL2 -DBL3 -DMASK -DBLUR --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cmb3.frag.qsb multieffect.frag
qsb -DBL1 -DBL2 -DBL3 -DMASK -DSHADOW --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cms3.frag.qsb multieffect.frag
qsb -DBL1 -DBL2 -DBL3 -DBLUR -DSHADOW --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cbs3.frag.qsb multieffect.frag
qsb -DBL1 -DBL2 -DBL3 -DMASK -DBLUR -DSHADOW --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cmbs3.frag.qsb multieffect.frag

:: Multieffect vertex shaders ::
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_c.vert.qsb multieffect.vert
qsb -DSHADOW -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o multieffect_cs.vert.qsb multieffect.vert

:: Bluritems shaders ::
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o bluritems.frag.qsb bluritems.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o bluritems.vert.qsb bluritems.vert
