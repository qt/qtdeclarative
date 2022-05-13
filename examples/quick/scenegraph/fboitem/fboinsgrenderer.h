// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FBOINSGRENDERER_H
#define FBOINSGRENDERER_H

#include <QtQuick/QQuickFramebufferObject>

class LogoRenderer;

class FboInSGRenderer : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Renderer)
public:
    Renderer *createRenderer() const;
};

#endif
