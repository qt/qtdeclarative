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

class QQuickOpenGLShaderEffect;
class QQuickGenericShaderEffect;
class QQuickShaderEffectPrivate;

class Q_QUICK_PRIVATE_EXPORT QQuickShaderEffect : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QByteArray fragmentShader READ fragmentShader WRITE setFragmentShader NOTIFY fragmentShaderChanged)
    Q_PROPERTY(QByteArray vertexShader READ vertexShader WRITE setVertexShader NOTIFY vertexShaderChanged)
    Q_PROPERTY(bool blending READ blending WRITE setBlending NOTIFY blendingChanged)
    Q_PROPERTY(QVariant mesh READ mesh WRITE setMesh NOTIFY meshChanged)
    Q_PROPERTY(CullMode cullMode READ cullMode WRITE setCullMode NOTIFY cullModeChanged)
    Q_PROPERTY(QString log READ log NOTIFY logChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool supportsAtlasTextures READ supportsAtlasTextures WRITE setSupportsAtlasTextures NOTIFY supportsAtlasTexturesChanged REVISION 4)
    QML_NAMED_ELEMENT(ShaderEffect)

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

    QByteArray fragmentShader() const;
    void setFragmentShader(const QByteArray &code);

    QByteArray vertexShader() const;
    void setVertexShader(const QByteArray &code);

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

#if QT_CONFIG(opengl)
    bool isOpenGLShaderEffect() const;
#endif

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
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData) override;
    void componentComplete() override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private:
#if QT_CONFIG(opengl)
    QQuickOpenGLShaderEffect *m_glImpl;
#endif
    QQuickGenericShaderEffect *m_impl;

    Q_DECLARE_PRIVATE(QQuickShaderEffect)
};

QT_END_NAMESPACE

#endif // QQUICKSHADEREFFECT_P_H
