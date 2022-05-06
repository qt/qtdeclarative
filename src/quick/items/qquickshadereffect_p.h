/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QQUICKSHADEREFFECT_P_H
#define QQUICKSHADEREFFECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_shadereffect);

#include <QtQuick/qquickitem.h>
#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickShaderEffectImpl;
class QQuickShaderEffectPrivate;

class Q_QUICK_PRIVATE_EXPORT QQuickShaderEffect : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QUrl fragmentShader READ fragmentShader WRITE setFragmentShader NOTIFY fragmentShaderChanged)
    Q_PROPERTY(QUrl vertexShader READ vertexShader WRITE setVertexShader NOTIFY vertexShaderChanged)
    Q_PROPERTY(bool blending READ blending WRITE setBlending NOTIFY blendingChanged)
    Q_PROPERTY(QVariant mesh READ mesh WRITE setMesh NOTIFY meshChanged)
    Q_PROPERTY(CullMode cullMode READ cullMode WRITE setCullMode NOTIFY cullModeChanged)
    Q_PROPERTY(QString log READ log NOTIFY logChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool supportsAtlasTextures READ supportsAtlasTextures WRITE setSupportsAtlasTextures NOTIFY supportsAtlasTexturesChanged REVISION(2, 4))
    QML_NAMED_ELEMENT(ShaderEffect)
    QML_ADDED_IN_VERSION(2, 0)

public:
    enum CullMode {
        NoCulling,
        BackFaceCulling,
        FrontFaceCulling
    };
    Q_ENUM(CullMode)

    enum Status {
        Compiled,
        Uncompiled,
        Error
    };
    Q_ENUM(Status)

    QQuickShaderEffect(QQuickItem *parent = nullptr);
    ~QQuickShaderEffect() override;

    QUrl fragmentShader() const;
    void setFragmentShader(const QUrl &fileUrl);

    QUrl vertexShader() const;
    void setVertexShader(const QUrl &fileUrl);

    bool blending() const;
    void setBlending(bool enable);

    QVariant mesh() const;
    void setMesh(const QVariant &mesh);

    CullMode cullMode() const;
    void setCullMode(CullMode face);

    bool supportsAtlasTextures() const;
    void setSupportsAtlasTextures(bool supports);

    QString log() const;
    Status status() const;

    bool isComponentComplete() const;
    QString parseLog();

    bool updateUniformValue(const QByteArray &name, const QVariant &value);

Q_SIGNALS:
    void fragmentShaderChanged();
    void vertexShaderChanged();
    void blendingChanged();
    void meshChanged();
    void cullModeChanged();
    void logChanged();
    void statusChanged();
    void supportsAtlasTexturesChanged();

protected:
    bool event(QEvent *e) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData) override;
    void componentComplete() override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private:
    QQuickShaderEffectImpl *m_impl;

    Q_DECLARE_PRIVATE(QQuickShaderEffect)
};

QT_END_NAMESPACE

#endif // QQUICKSHADEREFFECT_P_H
