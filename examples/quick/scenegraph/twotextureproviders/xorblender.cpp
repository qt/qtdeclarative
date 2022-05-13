// Copyright (C) 2014 Gunnar Sletta <gunnar@sletta.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "xorblender.h"

#include <QtCore/QPointer>

#include <QtQuick/QSGMaterial>
#include <QtQuick/QSGTexture>
#include <QtQuick/QSGGeometryNode>
#include <QtQuick/QSGTextureProvider>

/* This example could just as well have been implemented all in QML using
 * a ShaderEffect, and for 90% of all usecases, using a ShaderEffect will
 * be sufficient. This example exists to illustrate how to consume
 * texture providers from C++ and how to use multiple texture sources in
 * a custom material.
 */

class XorBlendMaterial : public QSGMaterial
{
public:
    XorBlendMaterial();
    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
    int compare(const QSGMaterial *other) const override;

    struct {
        QSGTexture *texture1 = nullptr;
        QSGTexture *texture2 = nullptr;
    } state;
};

class XorBlendRhiShader : public QSGMaterialShader
{
public:
    XorBlendRhiShader();
    bool updateUniformData(RenderState &state,
                           QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

XorBlendMaterial::XorBlendMaterial()
{
    setFlag(Blending);
}

QSGMaterialShader *XorBlendMaterial::createShader(QSGRendererInterface::RenderMode) const
{
    return new XorBlendRhiShader;
}

QSGMaterialType *XorBlendMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

int XorBlendMaterial::compare(const QSGMaterial *o) const
{
    Q_ASSERT(o && type() == o->type());
    const XorBlendMaterial *other = static_cast<const XorBlendMaterial *>(o);

    if (!state.texture1 || !other->state.texture1)
        return state.texture1 ? 1 : -1;

    if (!state.texture2 || !other->state.texture2)
        return state.texture2 ? -1 : 1;

    qint64 diff = state.texture1->comparisonKey() - other->state.texture1->comparisonKey();
    if (diff != 0)
        return diff < 0 ? -1 : 1;

    diff = state.texture2->comparisonKey() - other->state.texture2->comparisonKey();
    return diff < 0 ? -1 : (diff > 0 ? 1 : 0);
}

XorBlendRhiShader::XorBlendRhiShader()
{
    setShaderFileName(VertexStage, QLatin1String(":/scenegraph/twotextureproviders/shaders/xorblender.vert.qsb"));
    setShaderFileName(FragmentStage, QLatin1String(":/scenegraph/twotextureproviders/shaders/xorblender.frag.qsb"));
}

bool XorBlendRhiShader::updateUniformData(RenderState &state, QSGMaterial *, QSGMaterial *)
{
    bool changed = false;
    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 68);

    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
        changed = true;
    }

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 64, &opacity, 4);
        changed = true;
    }

    return changed;
}

void XorBlendRhiShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                           QSGMaterial *newMaterial, QSGMaterial *)
{
    Q_UNUSED(state);

    XorBlendMaterial *mat = static_cast<XorBlendMaterial *>(newMaterial);
    switch (binding) { // the binding for the sampler2Ds in the fragment shader
    case 1:
        *texture = mat->state.texture1;
        break;
    case 2:
        *texture = mat->state.texture2;
        break;
    default:
        return;
    }
}

/* The rendering is split into two nodes. The top-most node is not actually
 * rendering anything, but is responsible for managing the texture providers.
 * The XorNode also has a geometry node internally which it uses to render
 * the texture providers using the XorBlendShader when all providers and
 * textures are all present.
 *
 * The texture providers are updated solely on the render thread (when rendering
 * is happening on a separate thread). This is why we are using preprocess
 * and direct signals between the the texture providers and the node rather
 * than updating state in updatePaintNode.
 */
class XorNode : public QObject, public QSGNode
{
    Q_OBJECT
public:
    XorNode(QSGTextureProvider *p1, QSGTextureProvider *p2)
        : m_provider1(p1)
        , m_provider2(p2)
    {
        setFlag(QSGNode::UsePreprocess, true);

        // Set up material so it is all set for later..
        m_material = new XorBlendMaterial;
        m_node.setMaterial(m_material);
        m_node.setFlag(QSGNode::OwnsMaterial);

        // Set up geometry, actual vertices will be initialized in updatePaintNode
        m_node.setGeometry(new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4));
        m_node.setFlag(QSGNode::OwnsGeometry);

        // If this node is used as in a shader effect source, we need to propegate
        // changes that will occur in this node outwards.
        connect(m_provider1.data(), &QSGTextureProvider::textureChanged, this, &XorNode::textureChange, Qt::DirectConnection);
        connect(m_provider2.data(), &QSGTextureProvider::textureChanged, this, &XorNode::textureChange, Qt::DirectConnection);
    }

    void preprocess() override {
        // Update the textures from the providers, calling into QSGDynamicTexture if required
        if (m_provider1) {
            m_material->state.texture1 = m_provider1->texture();
            if (QSGDynamicTexture *dt1 = qobject_cast<QSGDynamicTexture *>(m_material->state.texture1))
                dt1->updateTexture();
        }
        if (m_provider2) {
            m_material->state.texture2 = m_provider2->texture();
            if (QSGDynamicTexture *dt2 = qobject_cast<QSGDynamicTexture *>(m_material->state.texture2))
                dt2->updateTexture();
        }

        // Remove node from the scene graph if it is there and either texture is missing...
        if (m_node.parent() && (!m_material->state.texture1 || !m_material->state.texture2))
            removeChildNode(&m_node);

        // Add it if it is not already there and both textures are present..
        else if (!m_node.parent() && m_material->state.texture1 && m_material->state.texture2)
            appendChildNode(&m_node);
    }

    void setRect(const QRectF &rect) {
        // Update geometry if it has changed and mark the change in the scene graph.
        if (m_rect != rect) {
            m_rect = rect;
            QSGGeometry::updateTexturedRectGeometry(m_node.geometry(), m_rect, QRectF(0, 0, 1, 1));
            m_node.markDirty(QSGNode::DirtyGeometry);
        }
    }

public slots:
    void textureChange() {
        // When our sources change, we will look different, so signal the change to the
        // scene graph.
        markDirty(QSGNode::DirtyMaterial);
    }

private:
    QRectF m_rect;
    XorBlendMaterial *m_material;
    QSGGeometryNode m_node;
    QPointer<QSGTextureProvider> m_provider1;
    QPointer<QSGTextureProvider> m_provider2;
};

XorBlender::XorBlender(QQuickItem *parent)
    : QQuickItem(parent)
    , m_source1(nullptr)
    , m_source2(nullptr)
    , m_source1Changed(false)
    , m_source2Changed(false)
{
    setFlag(ItemHasContents, true);
}

void XorBlender::setSource1(QQuickItem *i)
{
    if (i == m_source1)
        return;
    m_source1 = i;
    emit source1Changed(m_source1);
    m_source1Changed = true;
    update();
}

void XorBlender::setSource2(QQuickItem *i)
{
    if (i == m_source2)
        return;
    m_source2 = i;
    emit source2Changed(m_source2);
    m_source2Changed = true;
    update();
}

QSGNode *XorBlender::updatePaintNode(QSGNode *old, UpdatePaintNodeData *)
{
    // Check if our input is valid and abort if not, deleting the old node.
    bool abort = false;
    if (!m_source1 || !m_source1->isTextureProvider()) {
        qDebug() << "source1 is missing or not a texture provider";
        abort = true;
    }
    if (!m_source2 || !m_source2->isTextureProvider()) {
        qDebug() << "source2 is missing or not a texture provider";
        abort = true;
    }
    if (abort) {
        delete old;
        return nullptr;
    }

    XorNode *node = static_cast<XorNode *>(old);

    // If the sources have changed, recreate the nodes
    if (m_source1Changed || m_source2Changed) {
        delete node;
        node = nullptr;
        m_source1Changed = false;
        m_source2Changed = false;
    }

    // Create a new XorNode for us to render with.
    if (!node)
        node = new XorNode(m_source1->textureProvider(), m_source2->textureProvider());

    // Update the geometry of the node to match the new bounding rect
    node->setRect(boundingRect());

    return node;
}

#include "xorblender.moc"
