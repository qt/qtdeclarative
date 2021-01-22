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

#ifndef QSGMATERIALRHISHADER_H
#define QSGMATERIALRHISHADER_H

#include <QtQuick/qtquickglobal.h>
#include <QtCore/QRect>
#include <QtGui/QMatrix4x4>
#include <QtGui/QColor>
#include <QtQuick/qsgmaterialshader.h>

QT_BEGIN_NAMESPACE

class QSGMaterial;
class QSGMaterialRhiShaderPrivate;
class QSGTexture;
class QRhiResourceUpdateBatch;
class QRhi;
class QShader;

class Q_QUICK_EXPORT QSGMaterialRhiShader : public QSGMaterialShader // ### Qt 6: remove inheritance
{
public:
    class Q_QUICK_EXPORT RenderState {
    public:
        using DirtyStates = QSGMaterialShader::RenderState::DirtyStates;

        inline DirtyStates dirtyStates() const { return m_dirty; }

        inline bool isMatrixDirty() const { return m_dirty & QSGMaterialShader::RenderState::DirtyMatrix; }
        inline bool isOpacityDirty() const { return m_dirty & QSGMaterialShader::RenderState::DirtyOpacity; }

        float opacity() const;
        QMatrix4x4 combinedMatrix() const;
        QMatrix4x4 modelViewMatrix() const;
        QMatrix4x4 projectionMatrix() const;
        QRect viewportRect() const;
        QRect deviceRect() const;
        float determinant() const;
        float devicePixelRatio() const;

        QByteArray *uniformData();
        QRhiResourceUpdateBatch *resourceUpdateBatch();
        QRhi *rhi();

    private:
        friend class QSGRenderer;
        DirtyStates m_dirty;
        const void *m_data;
    };

    struct Q_QUICK_EXPORT GraphicsPipelineState {
        enum BlendFactor {
            Zero,
            One,
            SrcColor,
            OneMinusSrcColor,
            DstColor,
            OneMinusDstColor,
            SrcAlpha,
            OneMinusSrcAlpha,
            DstAlpha,
            OneMinusDstAlpha,
            ConstantColor,
            OneMinusConstantColor,
            ConstantAlpha,
            OneMinusConstantAlpha,
            SrcAlphaSaturate,
            Src1Color,
            OneMinusSrc1Color,
            Src1Alpha,
            OneMinusSrc1Alpha
        };

        enum ColorMaskComponent {
            R = 1 << 0,
            G = 1 << 1,
            B = 1 << 2,
            A = 1 << 3
        };
        Q_DECLARE_FLAGS(ColorMask, ColorMaskComponent)

        enum CullMode {
            CullNone,
            CullFront,
            CullBack
        };

        bool blendEnable;
        BlendFactor srcColor;
        BlendFactor dstColor;
        ColorMask colorWrite;
        QColor blendConstant;
        CullMode cullMode;
        // This struct is extensible while keeping BC since apps only ever get
        // a ptr to the struct, it is not created by them.
    };

    enum Flag {
        UpdatesGraphicsPipelineState = 0x0001
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    enum Stage {
        VertexStage,
        FragmentStage,
    };

    QSGMaterialRhiShader();
    virtual ~QSGMaterialRhiShader();

    virtual bool updateUniformData(RenderState &state,
                                   QSGMaterial *newMaterial, QSGMaterial *oldMaterial);

    virtual void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                    QSGMaterial *newMaterial, QSGMaterial *oldMaterial);

    virtual bool updateGraphicsPipelineState(RenderState &state, GraphicsPipelineState *ps,
                                             QSGMaterial *newMaterial, QSGMaterial *oldMaterial);

    Flags flags() const;
    void setFlag(Flags flags, bool on = true);

    // dummy impl for base class pure virtual, never called
    char const *const *attributeNames() const override;

protected:
    Q_DECLARE_PRIVATE(QSGMaterialRhiShader)
    QSGMaterialRhiShader(QSGMaterialRhiShaderPrivate &dd);

    // filename is for a file containing a serialized QShader.
    void setShaderFileName(Stage stage, const QString &filename);

    void setShader(Stage stage, const QShader &shader);

private:
    QScopedPointer<QSGMaterialRhiShaderPrivate> d_ptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGMaterialRhiShader::GraphicsPipelineState::ColorMask)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSGMaterialRhiShader::Flags)

QT_END_NAMESPACE

#endif
