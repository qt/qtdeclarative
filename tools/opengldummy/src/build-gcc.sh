#!/bin/sh
${CC} -DQGS_BUILD_CLIENT_DLL -fPIC -shared --sysroot ${SYSROOT} -I../3rdparty/include -o libEGL.so egl.cpp
${CC} -DQGS_BUILD_CLIENT_DLL -fPIC -shared --sysroot ${SYSROOT} -I../3rdparty/include -o libGLESv2.so gles2.cpp
