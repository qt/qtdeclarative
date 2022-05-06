:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: Copyright (C) 2022 The Qt Company Ltd.
:: Contact: https://www.qt.io/licensing/
::
:: This file is part of the QtQuick module of the Qt Toolkit.
::
:: $QT_BEGIN_LICENSE:COMM$
::
:: Commercial License Usage
:: Licensees holding valid commercial Qt licenses may use this file in
:: accordance with the commercial license agreement provided with the
:: Software or, alternatively, in accordance with the terms contained in
:: a written agreement between you and The Qt Company. For licensing terms
:: and conditions see https://www.qt.io/terms-conditions. For further
:: information use the contact form at https://www.qt.io/contact-us.
::
:: $QT_END_LICENSE$
::
::
::
::
::
::
::
::
::
::
::
::
::
::
::
::
::
::
::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:: For HLSL we invoke fxc.exe (-c argument) and store the resulting intermediate format
:: instead of HLSL source, so this needs to be run on Windows from a developer command prompt.

:: For SPIR-V the optimizer is requested (-O argument) which means spirv-opt must be
:: invokable (e.g. because it's in the PATH from the Vulkan SDK)

qsb -DPOINT -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o imageparticle_simplepoint.vert.qsb imageparticle.vert
qsb -DPOINT --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o imageparticle_simplepoint.frag.qsb imageparticle.frag

qsb -DPOINT -DCOLOR -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o imageparticle_coloredpoint.vert.qsb imageparticle.vert
qsb -DPOINT -DCOLOR --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o imageparticle_coloredpoint.frag.qsb imageparticle.frag

qsb -DCOLOR -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o imageparticle_colored.vert.qsb imageparticle.vert
qsb -DCOLOR --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o imageparticle_colored.frag.qsb imageparticle.frag

qsb -DDEFORM -DCOLOR -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o imageparticle_deformed.vert.qsb imageparticle.vert
qsb -DDEFORM -DCOLOR --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o imageparticle_deformed.frag.qsb imageparticle.frag

qsb -DTABLE -DDEFORM -DCOLOR -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o imageparticle_tabled.vert.qsb imageparticle.vert
qsb -DTABLE -DDEFORM -DCOLOR --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o imageparticle_tabled.frag.qsb imageparticle.frag

qsb -DSPRITE -DTABLE -DDEFORM -DCOLOR -b --zorder-loc 8 --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o imageparticle_sprite.vert.qsb imageparticle.vert
qsb -DSPRITE -DTABLE -DDEFORM -DCOLOR --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o imageparticle_sprite.frag.qsb imageparticle.frag


