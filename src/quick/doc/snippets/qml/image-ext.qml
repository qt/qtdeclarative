// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [ext]
// Assuming the "pics" directory contains the following files:
//   dog.jpg
//   cat.png
//   cat.pkm

Image {
    source: "pics/cat.png"     // loads cat.png
}

Image {
    source: "pics/dog"         // loads dog.jpg
}

Image {
    source: "pics/cat"         // normally loads cat.pkm, but if no OpenGL, loads cat.png instead.
}
//! [ext]
