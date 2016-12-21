/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickpathitemnvprrenderer_p.h"
#include <QOpenGLExtraFunctions>
#include <private/qquickpath_p_p.h>

QT_BEGIN_NAMESPACE

void QQuickPathItemNvprRenderer::beginSync()
{
    // nothing to do here
}

void QQuickPathItemNvprRenderer::setPath(const QQuickPath *path)
{
    convertPath(path);
    m_dirty |= DirtyPath;
}

void QQuickPathItemNvprRenderer::setStrokeColor(const QColor &color)
{
    m_strokeColor = color;
    m_dirty |= DirtyStyle;
}

void QQuickPathItemNvprRenderer::setStrokeWidth(qreal w)
{
    m_strokeWidth = w;
    m_dirty |= DirtyStyle;
}

void QQuickPathItemNvprRenderer::setFillColor(const QColor &color)
{
    m_fillColor = color;
    m_dirty |= DirtyStyle;
}

void QQuickPathItemNvprRenderer::setFillRule(QQuickPathItem::FillRule fillRule)
{
    m_fillRule = fillRule;
    m_dirty |= DirtyFillRule;
}

void QQuickPathItemNvprRenderer::setJoinStyle(QQuickPathItem::JoinStyle joinStyle, int miterLimit)
{
    m_joinStyle = joinStyle;
    m_miterLimit = miterLimit;
    m_dirty |= DirtyStyle;
}

void QQuickPathItemNvprRenderer::setCapStyle(QQuickPathItem::CapStyle capStyle)
{
    m_capStyle = capStyle;
    m_dirty |= DirtyStyle;
}

void QQuickPathItemNvprRenderer::setStrokeStyle(QQuickPathItem::StrokeStyle strokeStyle,
                                                   qreal dashOffset, const QVector<qreal> &dashPattern)
{
    m_dashActive = strokeStyle == QQuickPathItem::DashLine;
    m_dashOffset = dashOffset;
    m_dashPattern = dashPattern;
    m_dirty |= DirtyDash;
}

void QQuickPathItemNvprRenderer::setFillGradient(QQuickPathGradient *gradient)
{
    m_fillGradientActive = gradient != nullptr;
    if (gradient) {
        m_fillGradient.stops = gradient->sortedGradientStops();
        m_fillGradient.spread = gradient->spread();
        if (QQuickPathLinearGradient *g  = qobject_cast<QQuickPathLinearGradient *>(gradient)) {
            m_fillGradient.start = QPointF(g->x1(), g->y1());
            m_fillGradient.end = QPointF(g->x2(), g->y2());
        } else {
            Q_UNREACHABLE();
        }
    }
    m_dirty |= DirtyFillGradient;
}

void QQuickPathItemNvprRenderer::endSync()
{
    // nothing to do here
}

void QQuickPathItemNvprRenderer::setNode(QQuickPathItemNvprRenderNode *node)
{
    if (m_node != node) {
        m_node = node;
        // Scenegraph nodes can be destroyed and then replaced by new ones over
        // time; hence it is important to mark everything dirty for
        // updatePathRenderNode(). We can assume the renderer has a full sync
        // of the data at this point.
        m_dirty = DirtyAll;
    }
}

QDebug operator<<(QDebug debug, const QQuickPathItemNvprRenderer::NvprPath &path)
{
    QDebugStateSaver saver(debug);
    debug.space().noquote();
    debug << "Path with" << path.cmd.count() << "commands";
    int ci = 0;
    for (GLubyte cmd : path.cmd) {
        static struct { GLubyte cmd; const char *s; int coordCount; } nameTab[] = {
        { GL_MOVE_TO_NV, "moveTo", 2 },
        { GL_LINE_TO_NV, "lineTo", 2 },
        { GL_QUADRATIC_CURVE_TO_NV, "quadTo", 4 },
        { GL_CUBIC_CURVE_TO_NV, "cubicTo", 6 },
        { GL_LARGE_CW_ARC_TO_NV, "arcTo-large-CW", 5 },
        { GL_LARGE_CCW_ARC_TO_NV, "arcTo-large-CCW", 5 },
        { GL_SMALL_CW_ARC_TO_NV, "arcTo-small-CW", 5 },
        { GL_SMALL_CCW_ARC_TO_NV, "arcTo-small-CCW", 5 },
        { GL_CLOSE_PATH_NV, "closePath", 0 } };
        for (size_t i = 0; i < sizeof(nameTab) / sizeof(nameTab[0]); ++i) {
            if (nameTab[i].cmd == cmd) {
                QByteArray cs;
                for (int j = 0; j < nameTab[i].coordCount; ++j) {
                    cs.append(QByteArray::number(path.coord[ci++]));
                    cs.append(' ');
                }
                debug << "\n  " << nameTab[i].s << " " << cs;
                break;
            }
        }
    }
    return debug;
}

static inline void appendCoords(QVector<GLfloat> *v, QQuickCurve *c, QPointF *pos)
{
    QPointF p(c->hasRelativeX() ? pos->x() + c->relativeX() : c->x(),
              c->hasRelativeY() ? pos->y() + c->relativeY() : c->y());
    v->append(p.x());
    v->append(p.y());
    *pos = p;
}

static inline void appendControlCoords(QVector<GLfloat> *v, QQuickPathQuad *c, const QPointF &pos)
{
    QPointF p(c->hasRelativeControlX() ? pos.x() + c->relativeControlX() : c->controlX(),
              c->hasRelativeControlY() ? pos.y() + c->relativeControlY() : c->controlY());
    v->append(p.x());
    v->append(p.y());
}

static inline void appendControl1Coords(QVector<GLfloat> *v, QQuickPathCubic *c, const QPointF &pos)
{
    QPointF p(c->hasRelativeControl1X() ? pos.x() + c->relativeControl1X() : c->control1X(),
              c->hasRelativeControl1Y() ? pos.y() + c->relativeControl1Y() : c->control1Y());
    v->append(p.x());
    v->append(p.y());
}

static inline void appendControl2Coords(QVector<GLfloat> *v, QQuickPathCubic *c, const QPointF &pos)
{
    QPointF p(c->hasRelativeControl2X() ? pos.x() + c->relativeControl2X() : c->control2X(),
              c->hasRelativeControl2Y() ? pos.y() + c->relativeControl2Y() : c->control2Y());
    v->append(p.x());
    v->append(p.y());
}

void QQuickPathItemNvprRenderer::convertPath(const QQuickPath *path)
{
    m_path = NvprPath();
    if (!path)
        return;

    const QList<QQuickPathElement *> &pp(QQuickPathPrivate::get(path)->_pathElements);
    if (pp.isEmpty())
        return;

    QPointF pos(path->startX(), path->startY());
    m_path.cmd.append(GL_MOVE_TO_NV);
    m_path.coord.append(pos.x());
    m_path.coord.append(pos.y());

    for (QQuickPathElement *e : pp) {
        if (QQuickPathMove *o = qobject_cast<QQuickPathMove *>(e)) {
            m_path.cmd.append(GL_MOVE_TO_NV);
            appendCoords(&m_path.coord, o, &pos);
        } else if (QQuickPathLine *o = qobject_cast<QQuickPathLine *>(e)) {
            m_path.cmd.append(GL_LINE_TO_NV);
            appendCoords(&m_path.coord, o, &pos);
        } else if (QQuickPathQuad *o = qobject_cast<QQuickPathQuad *>(e)) {
            m_path.cmd.append(GL_QUADRATIC_CURVE_TO_NV);
            appendControlCoords(&m_path.coord, o, pos);
            appendCoords(&m_path.coord, o, &pos);
        } else if (QQuickPathCubic *o = qobject_cast<QQuickPathCubic *>(e)) {
            m_path.cmd.append(GL_CUBIC_CURVE_TO_NV);
            appendControl1Coords(&m_path.coord, o, pos);
            appendControl2Coords(&m_path.coord, o, pos);
            appendCoords(&m_path.coord, o, &pos);
        } else if (QQuickPathArc *o = qobject_cast<QQuickPathArc *>(e)) {
            const bool sweepFlag = o->direction() == QQuickPathArc::Clockwise; // maps to CCW, not a typo
            GLenum cmd;
            if (o->useLargeArc())
                cmd = sweepFlag ? GL_LARGE_CCW_ARC_TO_NV : GL_LARGE_CW_ARC_TO_NV;
            else
                cmd = sweepFlag ? GL_SMALL_CCW_ARC_TO_NV : GL_SMALL_CW_ARC_TO_NV;
            m_path.cmd.append(cmd);
            m_path.coord.append(o->radiusX());
            m_path.coord.append(o->radiusY());
            m_path.coord.append(0.0f); // X axis rotation
            appendCoords(&m_path.coord, o, &pos);
        } else {
            qWarning() << "PathItem/NVPR: unsupported Path element" << e;
        }
    }

    if (qFuzzyCompare(pos.x(), path->startX()) && qFuzzyCompare(pos.y(), path->startY()))
        m_path.cmd.append(GL_CLOSE_PATH_NV);
}

static inline QVector4D qsg_premultiply(const QColor &c, float globalOpacity)
{
    const float o = c.alphaF() * globalOpacity;
    return QVector4D(c.redF() * o, c.greenF() * o, c.blueF() * o, o);
}

void QQuickPathItemNvprRenderer::updatePathRenderNode()
{
    // Called on the render thread with gui blocked -> update the node with its
    // own copy of all relevant data.

    if (!m_dirty)
        return;

    // updatePathRenderNode() can be called several times with different dirty
    // state before render() gets invoked. So accumulate.
    m_node->m_dirty |= m_dirty;

    if (m_dirty & DirtyPath)
        m_node->m_source = m_path;

    if (m_dirty & DirtyStyle) {
        m_node->m_strokeWidth = m_strokeWidth;
        m_node->m_strokeColor = qsg_premultiply(m_strokeColor, 1.0f);
        m_node->m_fillColor = qsg_premultiply(m_fillColor, 1.0f);
        switch (m_joinStyle) {
        case QQuickPathItem::MiterJoin:
            m_node->m_joinStyle = GL_MITER_TRUNCATE_NV;
            break;
        case QQuickPathItem::BevelJoin:
            m_node->m_joinStyle = GL_BEVEL_NV;
            break;
        case QQuickPathItem::RoundJoin:
            m_node->m_joinStyle = GL_ROUND_NV;
            break;
        default:
            Q_UNREACHABLE();
        }
        m_node->m_miterLimit = m_miterLimit;
        switch (m_capStyle) {
        case QQuickPathItem::FlatCap:
            m_node->m_capStyle = GL_FLAT;
            break;
        case QQuickPathItem::SquareCap:
            m_node->m_capStyle = GL_SQUARE_NV;
            break;
        case QQuickPathItem::RoundCap:
            m_node->m_capStyle = GL_ROUND_NV;
            break;
        default:
            Q_UNREACHABLE();
        }
    }

    if (m_dirty & DirtyFillRule) {
        switch (m_fillRule) {
        case QQuickPathItem::OddEvenFill:
            m_node->m_fillRule = GL_COUNT_UP_NV;
            break;
        case QQuickPathItem::WindingFill:
            m_node->m_fillRule = GL_INVERT;
            break;
        default:
            Q_UNREACHABLE();
        }
    }

    if (m_dirty & DirtyDash) {
        m_node->m_dashOffset = m_dashOffset;
        if (m_dashActive) {
            m_node->m_dashPattern.resize(m_dashPattern.count());
            // Multiply by strokeWidth because the PathItem API follows QPen
            // meaning the input dash pattern here is in width units.
            for (int i = 0; i < m_dashPattern.count(); ++i)
                m_node->m_dashPattern[i] = GLfloat(m_dashPattern[i]) * m_strokeWidth;
        } else {
            m_node->m_dashPattern.clear();
        }
    }

    if (m_dirty & DirtyFillGradient) {
        m_node->m_fillGradientActive = m_fillGradientActive;
        if (m_fillGradientActive)
            m_node->m_fillGradient = m_fillGradient;
    }

    m_node->markDirty(QSGNode::DirtyMaterial);
    m_dirty = 0;
}

bool QQuickPathItemNvprRenderNode::nvprInited = false;
QQuickNvprFunctions QQuickPathItemNvprRenderNode::nvpr;
QQuickNvprMaterialManager QQuickPathItemNvprRenderNode::mtlmgr;

QQuickPathItemNvprRenderNode::QQuickPathItemNvprRenderNode(QQuickPathItem *item)
    : m_item(item)
{
}

QQuickPathItemNvprRenderNode::~QQuickPathItemNvprRenderNode()
{
    releaseResources();
}

void QQuickPathItemNvprRenderNode::releaseResources()
{
    if (m_path) {
        nvpr.deletePaths(m_path, 1);
        m_path = 0;
    }
}

void QQuickNvprMaterialManager::create(QQuickNvprFunctions *nvpr)
{
    m_nvpr = nvpr;
}

void QQuickNvprMaterialManager::releaseResources()
{
    QOpenGLExtraFunctions *f = QOpenGLContext::currentContext()->extraFunctions();
    for (MaterialDesc &mtl : m_materials) {
        if (mtl.ppl) {
            f->glDeleteProgramPipelines(1, &mtl.ppl);
            mtl = MaterialDesc();
        }
    }
}

QQuickNvprMaterialManager::MaterialDesc *QQuickNvprMaterialManager::activateMaterial(Material m)
{
    QOpenGLExtraFunctions *f = QOpenGLContext::currentContext()->extraFunctions();
    MaterialDesc &mtl(m_materials[m]);

    if (!mtl.ppl) {
        if (m == MatSolid) {
            static const char *fragSrc =
                    "#version 310 es\n"
                    "precision highp float;\n"
                    "out vec4 fragColor;\n"
                    "uniform vec4 color;\n"
                    "uniform float opacity;\n"
                    "void main() {\n"
                    "  fragColor = color * opacity;\n"
                    "}\n";
            if (!m_nvpr->createFragmentOnlyPipeline(fragSrc, &mtl.ppl, &mtl.prg)) {
                qWarning("NVPR: Failed to create shader pipeline for solid fill");
                return nullptr;
            }
            Q_ASSERT(mtl.ppl && mtl.prg);
            mtl.uniLoc[0] = f->glGetProgramResourceLocation(mtl.prg, GL_UNIFORM, "color");
            Q_ASSERT(mtl.uniLoc[0] >= 0);
            mtl.uniLoc[1] = f->glGetProgramResourceLocation(mtl.prg, GL_UNIFORM, "opacity");
            Q_ASSERT(mtl.uniLoc[1] >= 0);
        } else if (m == MatLinearGradient) {
            static const char *fragSrc =
                    "#version 310 es\n"
                    "precision highp float;\n"
                    "layout(location = 0) in vec2 uv;"
                    "uniform float opacity;\n"
                    "uniform sampler2D gradTab;\n"
                    "uniform vec2 gradStart;\n"
                    "uniform vec2 gradEnd;\n"
                    "out vec4 fragColor;\n"
                    "void main() {\n"
                    "  vec2 gradVec = gradEnd - gradStart;\n"
                    "  float gradTabIndex = dot(gradVec, uv - gradStart) / (gradVec.x * gradVec.x + gradVec.y * gradVec.y);\n"
                    "  fragColor = texture(gradTab, vec2(gradTabIndex, 0.5)) * opacity;\n"
                    "}\n";
            if (!m_nvpr->createFragmentOnlyPipeline(fragSrc, &mtl.ppl, &mtl.prg)) {
                qWarning("NVPR: Failed to create shader pipeline for linear gradient");
                return nullptr;
            }
            Q_ASSERT(mtl.ppl && mtl.prg);
            mtl.uniLoc[1] = f->glGetProgramResourceLocation(mtl.prg, GL_UNIFORM, "opacity");
            Q_ASSERT(mtl.uniLoc[1] >= 0);
            mtl.uniLoc[2] = f->glGetProgramResourceLocation(mtl.prg, GL_UNIFORM, "gradStart");
            Q_ASSERT(mtl.uniLoc[2] >= 0);
            mtl.uniLoc[3] = f->glGetProgramResourceLocation(mtl.prg, GL_UNIFORM, "gradEnd");
            Q_ASSERT(mtl.uniLoc[3] >= 0);
        } else {
            Q_UNREACHABLE();
        }
    }

    f->glBindProgramPipeline(mtl.ppl);

    return &mtl;
}

void QQuickPathItemNvprRenderNode::updatePath()
{
    if (m_dirty & QQuickPathItemNvprRenderer::DirtyPath) {
        if (!m_path) {
            m_path = nvpr.genPaths(1);
            Q_ASSERT(m_path != 0);
        }
        nvpr.pathCommands(m_path, m_source.cmd.count(), m_source.cmd.constData(),
                          m_source.coord.count(), GL_FLOAT, m_source.coord.constData());
    }

    if (m_dirty & QQuickPathItemNvprRenderer::DirtyStyle) {
        nvpr.pathParameterf(m_path, GL_PATH_STROKE_WIDTH_NV, m_strokeWidth);
        nvpr.pathParameteri(m_path, GL_PATH_JOIN_STYLE_NV, m_joinStyle);
        nvpr.pathParameteri(m_path, GL_PATH_MITER_LIMIT_NV, m_miterLimit);
        nvpr.pathParameteri(m_path, GL_PATH_END_CAPS_NV, m_capStyle);
        nvpr.pathParameteri(m_path, GL_PATH_DASH_CAPS_NV, m_capStyle);
    }

    if (m_dirty & QQuickPathItemNvprRenderer::DirtyDash) {
        nvpr.pathParameterf(m_path, GL_PATH_DASH_OFFSET_NV, m_dashOffset);
        // count == 0 -> no dash
        nvpr.pathDashArray(m_path, m_dashPattern.count(), m_dashPattern.constData());
    }
}

void QQuickPathItemNvprRenderNode::render(const RenderState *state)
{
    if (!nvprInited) {
        if (!nvpr.create()) {
            qWarning("NVPR init failed");
            return;
        }
        mtlmgr.create(&nvpr);
        nvprInited = true;
    }

    updatePath();

    QOpenGLExtraFunctions *f = QOpenGLContext::currentContext()->extraFunctions();
    f->glUseProgram(0);
    QQuickNvprMaterialManager::MaterialDesc *mtl;
    if (m_fillGradientActive)
        mtl = mtlmgr.activateMaterial(QQuickNvprMaterialManager::MatLinearGradient);
    else
        mtl = mtlmgr.activateMaterial(QQuickNvprMaterialManager::MatSolid);
    if (!mtl)
        return;

    // Assume stencil buffer is cleared to 0 for each frame.
    // Within the frame dppass=GL_ZERO for glStencilOp ensures stencil is reset and so no need to clear.
    f->glStencilMask(~0);
    f->glEnable(GL_STENCIL_TEST);
    f->glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
    f->glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

    // Depth test against the opaque batches rendered before.
    f->glEnable(GL_DEPTH_TEST);
    f->glDepthFunc(GL_LESS);
    nvpr.pathCoverDepthFunc(GL_LESS);
    nvpr.pathStencilDepthOffset(-0.05f, -1);

    nvpr.matrixLoadf(GL_PATH_MODELVIEW_NV, matrix()->constData());
    nvpr.matrixLoadf(GL_PATH_PROJECTION_NV, state->projectionMatrix()->constData());

    if (state->scissorEnabled()) {
        // scissor rect is already set, just enable scissoring
        f->glEnable(GL_SCISSOR_TEST);
    }

    if (!qFuzzyIsNull(m_fillColor.w()) || m_fillGradientActive) {
        if (m_fillGradientActive) {
            QSGTexture *tx = QQuickPathItemGradientCache::currentCache()->get(m_fillGradient);
            tx->bind();
            // uv = vec2(coeff[0] * x + coeff[1] * y + coeff[2], coeff[3] * x + coeff[4] * y + coeff[5])
            // where x and y are in path coordinate space, which is just what
            // we need since the gradient's start and stop are in that space too.
            GLfloat coeff[6] = { 1, 0, 0,
                                 0, 1, 0 };
            nvpr.programPathFragmentInputGen(mtl->prg, 0, GL_OBJECT_LINEAR_NV, 2, coeff);
            f->glProgramUniform2f(mtl->prg, mtl->uniLoc[2], m_fillGradient.start.x(), m_fillGradient.start.y());
            f->glProgramUniform2f(mtl->prg, mtl->uniLoc[3], m_fillGradient.end.x(), m_fillGradient.end.y());
        } else {
            f->glProgramUniform4f(mtl->prg, mtl->uniLoc[0],
                    m_fillColor.x(), m_fillColor.y(), m_fillColor.z(), m_fillColor.w());
        }
        f->glProgramUniform1f(mtl->prg, mtl->uniLoc[1], inheritedOpacity());
        nvpr.stencilThenCoverFillPath(m_path, m_fillRule, 0xFF, GL_BOUNDING_BOX_NV);
    }

    if (m_strokeWidth >= 0.0f && !qFuzzyIsNull(m_strokeColor.w())) {
        if (m_fillGradientActive)
            mtl = mtlmgr.activateMaterial(QQuickNvprMaterialManager::MatSolid);
        f->glProgramUniform4f(mtl->prg, mtl->uniLoc[0],
                m_strokeColor.x(), m_strokeColor.y(), m_strokeColor.z(), m_strokeColor.w());
        f->glProgramUniform1f(mtl->prg, mtl->uniLoc[1], inheritedOpacity());
        nvpr.stencilThenCoverStrokePath(m_path, 0x1, ~0, GL_CONVEX_HULL_NV);
    }

    f->glBindProgramPipeline(0);

    m_dirty = 0;
}

QSGRenderNode::StateFlags QQuickPathItemNvprRenderNode::changedStates() const
{
    return BlendState | StencilState | DepthState | ScissorState;
}

QSGRenderNode::RenderingFlags QQuickPathItemNvprRenderNode::flags() const
{
    return DepthAwareRendering; // avoid hitting the less optimal no-opaque-batch path in the renderer
}

QRectF QQuickPathItemNvprRenderNode::rect() const
{
    return QRect(0, 0, m_item->width(), m_item->height());
}

bool QQuickPathItemNvprRenderNode::isSupported()
{
    static const bool nvprDisabled = qEnvironmentVariableIntValue("QT_NO_NVPR") != 0;
    return !nvprDisabled && QQuickNvprFunctions::isSupported();
}

QT_END_NAMESPACE
