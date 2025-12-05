// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquicksaturationlightnesspicker_p.h"
#include "qquickabstractcolorpicker_p_p.h"
#include "qsgsimpletexturenode.h"
#include "qsgtexture.h"

#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#include <QtQuickTemplates2/private/qquickdeferredexecute_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickSaturationLightnessPickerPrivate : public QQuickAbstractColorPickerPrivate
{
    Q_DECLARE_PUBLIC(QQuickSaturationLightnessPicker)

public:
    explicit QQuickSaturationLightnessPickerPrivate();
};
QQuickSaturationLightnessPickerPrivate::QQuickSaturationLightnessPickerPrivate()
{
    m_hsl = true;
}

class QQuickSaturationLightnessPickerMaterialShader : public QSGMaterialShader
{
public:
    QQuickSaturationLightnessPickerMaterialShader() {
        setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/imports/QtQuick/Dialogs/quickimpl/shaders/SaturationLightness.vert.qsb"));
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/imports/QtQuick/Dialogs/quickimpl/shaders/SaturationLightness.frag.qsb"));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

class QQuickSaturationLightnessPickerMaterial : public QSGMaterial
{
public:
    QQuickSaturationLightnessPickerMaterial() { setFlag(Blending, true); }
    QSGMaterialType* type() const override { static QSGMaterialType t; return &t; }
    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode renderMode) const override{
        Q_UNUSED(renderMode);
        return new QQuickSaturationLightnessPickerMaterialShader();
    }
    float hue = 0.0f;
};

bool QQuickSaturationLightnessPickerMaterialShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_ASSERT(!oldMaterial || newMaterial->type() == oldMaterial->type());

    QQuickSaturationLightnessPickerMaterial *mat =
            static_cast<QQuickSaturationLightnessPickerMaterial*>(newMaterial);
    QQuickSaturationLightnessPickerMaterial *oldMat =
            static_cast<QQuickSaturationLightnessPickerMaterial*>(oldMaterial);

    bool changed = false;
    QByteArray *buf = state.uniformData();

    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
        changed = true;
    }

    if (state.isOpacityDirty() || !oldMat) {
        float opacity = state.opacity();
        memcpy(buf->data() + 64, &opacity, sizeof(float));
        changed = true;
    }

    if (!oldMat || mat->hue != oldMat->hue) {
        float hue = mat->hue;
        memcpy(buf->data() + 64 + 4, &hue, sizeof(float));
        changed = true;
    }

    return changed;
}

QQuickSaturationLightnessPickerCanvas::QQuickSaturationLightnessPickerCanvas(QQuickItem *parent)
    : QQuickItem(parent), m_hue(0) {
    setFlag(ItemHasContents, true);
}

void QQuickSaturationLightnessPickerCanvas::setHue(qreal h)
{
    if (h == m_hue)
        return;
    m_hue = h;
    m_image = QImage();
    emit hueChanged();
    update();
}

QSGNode *QQuickSaturationLightnessPickerCanvas::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGGeometryNode *node = nullptr;

    bool shaderSupported = false;
    if (window()) {
        QSGRendererInterface::GraphicsApi api = window()->rendererInterface()->graphicsApi();
        switch (api) {
        case QSGRendererInterface::OpenGL:
        case QSGRendererInterface::Vulkan:
        case QSGRendererInterface::Metal:
        case QSGRendererInterface::Direct3D11:
            shaderSupported = true;
            break;
        default:
            shaderSupported = false;
        }
    }
    if (shaderSupported) {
        // GPU path
        QSGGeometryNode* geomNode = static_cast<QSGGeometryNode*>(oldNode);
        QSGGeometry* geom;
        if (!geomNode) {
            geomNode = new QSGGeometryNode();
            geom = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);
            geom->setDrawingMode(QSGGeometry::DrawTriangleStrip);
            geomNode->setGeometry(geom);
            geomNode->setFlag(QSGNode::OwnsGeometry);

            auto* mat = new QQuickSaturationLightnessPickerMaterial();
            geomNode->setMaterial(mat);
            geomNode->setFlag(QSGNode::OwnsMaterial);
        }
        else {
            geom = geomNode->geometry();
        }

        if (m_lastSize != size()) {
            m_lastSize = size();

            auto *v = geom->vertexDataAsTexturedPoint2D();
            v[0].set(0,          0,          0, 0);
            v[1].set(width(),    0,          1, 0);
            v[2].set(0,          height(),   0, 1);
            v[3].set(width(),    height(),   1, 1);

            geomNode->markDirty(QSGNode::DirtyGeometry);
        }

        auto* mat = static_cast<QQuickSaturationLightnessPickerMaterial*>(geomNode->material());
        if (mat->hue != m_hue) {
            mat->hue = m_hue;
            geomNode->markDirty(QSGNode::DirtyMaterial);
        }
        node = geomNode;

    } else {
        // CPU path
        QSGSimpleTextureNode *texNode = static_cast<QSGSimpleTextureNode*>(oldNode);
        if (!texNode) texNode = new QSGSimpleTextureNode();
        if (m_image.size().isNull() || m_image.size() != QSize(width(), height()))
            m_image = generateImage(width(), height(), m_hue);
        QSGTexture *tex = window()->createTextureFromImage(m_image);
        texNode->setTexture(tex);
        texNode->setOwnsTexture(true);
        texNode->setRect(0,0,width(),height());
        node = texNode;
    }
    return node;
}

QImage QQuickSaturationLightnessPickerCanvas::generateImage(int width, int height, double hue01) const
{
    QImage img(width, height, QImage::Format_RGB32);
    QRgb* bits = reinterpret_cast<QRgb*>(img.bits());
    double hue = hue01 * 360.0;

    // Pre-calculate RGB values for each saturation-lightness pair
    std::vector<QRgb> lut(256 * 256);
    for (int s = 0; s < 256; ++s) {
        for (int l = 0; l < 256; ++l) {
            lut[s * 256 + l] = QColor::fromHsl(hue, s, l).rgb();
        }
    }

    for (int y = 0; y < height; ++y) {
        int saturation = static_cast<int>((1.0 - double(y) / (height - 1)) * 255);
        QRgb* row = bits + y * width;
        for (int x = 0; x < width; ++x) {
            int lightness = static_cast<int>(double(x) / (width - 1) * 255);
            row[x] = lut[saturation * 256 + lightness];
        }
    }
    return img;
}

QQuickSaturationLightnessPicker::QQuickSaturationLightnessPicker(QQuickItem *parent)
    : QQuickAbstractColorPicker(*(new QQuickSaturationLightnessPickerPrivate), parent)
{
}

QColor QQuickSaturationLightnessPicker::colorAt(const QPointF &pos)
{
    const qreal w = width();
    const qreal h = height();
    if (w <= 0 || h <= 0)
        return color();
    const qreal x = qBound(.0, pos.x(), w);
    const qreal y = qBound(.0, pos.y(), h);
    const qreal saturation = 1.0 - (y / h);
    const qreal lightness = x / w;

    return QColor::fromHslF(hue(), saturation, lightness);
}

QT_END_NAMESPACE
