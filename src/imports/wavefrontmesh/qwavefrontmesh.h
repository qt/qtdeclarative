/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QWAVEFRONTMESH_H
#define QWAVEFRONTMESH_H

#include <QtQuick/private/qquickshadereffectmesh_p.h>

#include <QtCore/qurl.h>
#include <QtGui/qvector3d.h>

QT_BEGIN_NAMESPACE

class QWavefrontMeshPrivate;
class QWavefrontMesh : public QQuickShaderEffectMesh
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(Error lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QVector3D projectionPlaneV READ projectionPlaneV WRITE setProjectionPlaneV NOTIFY projectionPlaneVChanged)
    Q_PROPERTY(QVector3D projectionPlaneW READ projectionPlaneW WRITE setProjectionPlaneW NOTIFY projectionPlaneWChanged)
    QML_NAMED_ELEMENT(WavefrontMesh)

public:
    enum Error {
        NoError,
        InvalidSourceError,
        UnsupportedFaceShapeError,
        UnsupportedIndexSizeError,
        FileNotFoundError,
        NoAttributesError,
        MissingPositionAttributeError,
        MissingTextureCoordinateAttributeError,
        MissingPositionAndTextureCoordinateAttributesError,
        TooManyAttributesError,
        InvalidPlaneDefinitionError
    };
    Q_ENUMS(Error)

    QWavefrontMesh(QObject *parent = nullptr);
    ~QWavefrontMesh() override;

    QUrl source() const;
    void setSource(const QUrl &url);

    Error lastError() const;
    void setLastError(Error lastError);

    bool validateAttributes(const QVector<QByteArray> &attributes, int *posIndex) override;
    QSGGeometry *updateGeometry(QSGGeometry *geometry, int attrCount, int posIndex,
                                const QRectF &srcRect, const QRectF &rect) override;
    QString log() const override;

    QVector3D projectionPlaneV() const;
    void setProjectionPlaneV(const QVector3D &projectionPlaneV);

    QVector3D projectionPlaneW() const;
    void setProjectionPlaneW(const QVector3D &projectionPlaneW);

Q_SIGNALS:
    void sourceChanged();
    void lastErrorChanged();
    void projectionPlaneVChanged();
    void projectionPlaneWChanged();

protected Q_SLOTS:
    void readData();

private:
    Q_DISABLE_COPY(QWavefrontMesh)
    Q_DECLARE_PRIVATE(QWavefrontMesh)
};

QT_END_NAMESPACE

#endif // QWAVEFRONTGEOMETRYMODEL_H
