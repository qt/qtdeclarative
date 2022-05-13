// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "linenode.h"

#include <QtGui/QColor>

#include <QtQuick/QSGMaterial>

class LineShader : public QSGMaterialShader
{
public:
    LineShader() {
        setShaderFileName(VertexStage, QLatin1String(":/scenegraph/graph/shaders/line.vert.qsb"));
        setShaderFileName(FragmentStage, QLatin1String(":/scenegraph/graph/shaders/line.frag.qsb"));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

class LineMaterial : public QSGMaterial
{
public:
    LineMaterial()
    {
        setFlag(Blending);
    }

    QSGMaterialType *type() const override
    {
        static QSGMaterialType type;
        return &type;
    }

    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override
    {
        return new LineShader;
    }

    int compare(const QSGMaterial *m) const override
    {
        const LineMaterial *other = static_cast<const LineMaterial *>(m);

        if (int diff = int(state.color.rgb()) - int(other->state.color.rgb()))
            return diff;

        if (int diff = state.size - other->state.size)
            return diff;

        if (int diff = state.spread - other->state.spread)
            return diff;

        return 0;
    }

    struct {
        QColor color;
        float size;
        float spread;
    } state;
};

bool LineShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *)
{
    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 92);

    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
    }

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 80, &opacity, 4);
    }

    LineMaterial *mat = static_cast<LineMaterial *>(newMaterial);
    float c[4];
    mat->state.color.getRgbF(&c[0], &c[1], &c[2], &c[3]);
    memcpy(buf->data() + 64, c, 16);
    memcpy(buf->data() + 84, &mat->state.size, 4);
    memcpy(buf->data() + 88, &mat->state.spread, 4);

    return true;
}

struct LineVertex {
    float x;
    float y;
    float t;
    inline void set(float xx, float yy, float tt) { x = xx; y = yy; t = tt; }
};

static const QSGGeometry::AttributeSet &attributes()
{
    static QSGGeometry::Attribute attr[] = {
        QSGGeometry::Attribute::create(0, 2, QSGGeometry::FloatType, true),
        QSGGeometry::Attribute::create(1, 1, QSGGeometry::FloatType)
    };
    static QSGGeometry::AttributeSet set = { 2, 3 * sizeof(float), attr };
    return set;
}

LineNode::LineNode(float size, float spread, const QColor &color)
    : m_geometry(attributes(), 0)
{
    setGeometry(&m_geometry);
    m_geometry.setDrawingMode(QSGGeometry::DrawTriangleStrip);

    LineMaterial *m = new LineMaterial;
    m->state.color = color;
    m->state.size = size;
    m->state.spread = spread;

    setMaterial(m);
    setFlag(OwnsMaterial);
}

/*
 * Assumes that samples have values in the range of 0 to 1 and scales them to
 * the height of bounds. The samples are stretched out horizontally along the
 * width of the bounds.
 *
 * The position of each pair of points is identical, but we use the third value
 * "t" to shift the point up or down and to add antialiasing.
 */
void LineNode::updateGeometry(const QRectF &bounds, const QList<qreal> &samples)
{
    m_geometry.allocate(samples.size() * 2);

    float x = bounds.x();
    float y = bounds.y();
    float w = bounds.width();
    float h = bounds.height();

    float dx = w / (samples.size() - 1);

    LineVertex *v = (LineVertex *) m_geometry.vertexData();
    for (int i=0; i<samples.size(); ++i) {
        v[i*2+0].set(x + dx * i, y + samples.at(i) * h, 0);
        v[i*2+1].set(x + dx * i, y + samples.at(i) * h, 1);
    }

    markDirty(QSGNode::DirtyGeometry);
}
