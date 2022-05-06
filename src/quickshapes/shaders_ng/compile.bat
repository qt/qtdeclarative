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

qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o lineargradient.vert.qsb lineargradient.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o lineargradient.frag.qsb lineargradient.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o radialgradient.vert.qsb radialgradient.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o radialgradient.frag.qsb radialgradient.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o conicalgradient.vert.qsb conicalgradient.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o conicalgradient.frag.qsb conicalgradient.frag
