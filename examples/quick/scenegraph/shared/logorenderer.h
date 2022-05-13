// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef LOGORENDERER_H
#define LOGORENDERER_H

#include <QtGui/qvector3d.h>
#include <QtGui/qmatrix4x4.h>
#include <qopenglshaderprogram.h>
#include <qopenglfunctions.h>

#include <QTime>
#include <QList>

class LogoRenderer : protected QOpenGLFunctions
{
public:
    LogoRenderer();
    ~LogoRenderer();

    void render();
    void initialize();

private:

    qreal   m_fAngle;
    qreal   m_fScale;

    void paintQtLogo();
    void createGeometry();
    void quad(qreal x1, qreal y1, qreal x2, qreal y2, qreal x3, qreal y3, qreal x4, qreal y4);
    void extrude(qreal x1, qreal y1, qreal x2, qreal y2);

    QList<QVector3D> vertices;
    QList<QVector3D> normals;
    QOpenGLShaderProgram program1;
    int vertexAttr1;
    int normalAttr1;
    int matrixUniform1;
};
#endif
