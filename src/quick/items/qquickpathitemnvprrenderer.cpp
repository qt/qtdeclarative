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
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
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
            m_node->m_fillRule = GL_INVERT;
            break;
        case QQuickPathItem::WindingFill:
            m_node->m_fillRule = GL_COUNT_UP_NV;
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

    if (m_fallbackFbo) {
        delete m_fallbackFbo;
        m_fallbackFbo = nullptr;
    }

    m_fallbackBlitter.destroy();
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

void QQuickPathItemNvprRenderNode::renderStroke(int strokeStencilValue, int writeMask)
{
    QQuickNvprMaterialManager::MaterialDesc *mtl = mtlmgr.activateMaterial(QQuickNvprMaterialManager::MatSolid);
    f->glProgramUniform4f(mtl->prg, mtl->uniLoc[0],
            m_strokeColor.x(), m_strokeColor.y(), m_strokeColor.z(), m_strokeColor.w());
    f->glProgramUniform1f(mtl->prg, mtl->uniLoc[1], inheritedOpacity());

    nvpr.stencilThenCoverStrokePath(m_path, strokeStencilValue, writeMask, GL_CONVEX_HULL_NV);
}

void QQuickPathItemNvprRenderNode::renderFill()
{
    QQuickNvprMaterialManager::MaterialDesc *mtl = nullptr;
    if (m_fillGradientActive) {
        mtl = mtlmgr.activateMaterial(QQuickNvprMaterialManager::MatLinearGradient);
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
        mtl = mtlmgr.activateMaterial(QQuickNvprMaterialManager::MatSolid);
        f->glProgramUniform4f(mtl->prg, mtl->uniLoc[0],
                m_fillColor.x(), m_fillColor.y(), m_fillColor.z(), m_fillColor.w());
    }
    f->glProgramUniform1f(mtl->prg, mtl->uniLoc[1], inheritedOpacity());

    const int writeMask = 0xFF;
    nvpr.stencilThenCoverFillPath(m_path, m_fillRule, writeMask, GL_BOUNDING_BOX_NV);
}

void QQuickPathItemNvprRenderNode::renderOffscreenFill()
{
    QQuickWindow *w = m_item->window();
    const qreal dpr = w->effectiveDevicePixelRatio();
    QSize itemSize = QSize(m_item->width(), m_item->height()) * dpr;
    QSize rtSize = w->renderTargetSize();
    if (rtSize.isEmpty())
        rtSize = w->size() * dpr;

    if (m_fallbackFbo && m_fallbackFbo->size() != itemSize) {
        delete m_fallbackFbo;
        m_fallbackFbo = nullptr;
    }
    if (!m_fallbackFbo)
        m_fallbackFbo = new QOpenGLFramebufferObject(itemSize, QOpenGLFramebufferObject::CombinedDepthStencil);
    if (!m_fallbackFbo->bind())
        return;

    f->glViewport(0, 0, itemSize.width(), itemSize.height());
    f->glClearColor(0, 0, 0, 0);
    f->glClearStencil(0);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    f->glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
    f->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    nvpr.matrixLoadIdentity(GL_PATH_MODELVIEW_NV);
    QMatrix4x4 proj;
    proj.ortho(0, itemSize.width(), itemSize.height(), 0, 1, -1);
    nvpr.matrixLoadf(GL_PATH_PROJECTION_NV, proj.constData());

    renderFill();

    m_fallbackFbo->release();
    f->glViewport(0, 0, rtSize.width(), rtSize.height());
}

void QQuickPathItemNvprRenderNode::setupStencilForCover(bool stencilClip, int sv)
{
    if (!stencilClip) {
        // Assume stencil buffer is cleared to 0 for each frame.
        // Within the frame dppass=GL_ZERO for glStencilOp ensures stencil is reset and so no need to clear.
        f->glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
        f->glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
    } else {
        f->glStencilFunc(GL_LESS, sv, 0xFF); // pass if (sv & 0xFF) < (stencil_value & 0xFF)
        f->glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); // dppass: replace with the original value (clip's stencil ref value)
    }
}

void QQuickPathItemNvprRenderNode::render(const RenderState *state)
{
    f = QOpenGLContext::currentContext()->extraFunctions();

    if (!nvprInited) {
        if (!nvpr.create()) {
            qWarning("NVPR init failed");
            return;
        }
        mtlmgr.create(&nvpr);
        nvprInited = true;
    }

    updatePath();

    f->glUseProgram(0);
    f->glStencilMask(~0);
    f->glEnable(GL_STENCIL_TEST);

    const bool stencilClip = state->stencilEnabled();
    // when true, the stencil buffer already has a clip path with a ref value of sv
    const int sv = state->stencilValue();

    const bool hasFill = !qFuzzyIsNull(m_fillColor.w()) || m_fillGradientActive;
    const bool hasStroke = m_strokeWidth >= 0.0f && !qFuzzyIsNull(m_strokeColor.w());

    if (hasFill && stencilClip) {
        // Fall back to a texture when complex clipping is in use and we have
        // to fill. Reconciling glStencilFillPath's and the scenegraph's clip
        // stencil semantics has not succeeded so far...
        renderOffscreenFill();
    }

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

    // Fill!
    if (hasFill) {
        if (!stencilClip) {
            setupStencilForCover(false, 0);
            renderFill();
        } else {
            if (!m_fallbackBlitter.isCreated())
                m_fallbackBlitter.create();
            f->glStencilFunc(GL_EQUAL, sv, 0xFF);
            f->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            m_fallbackBlitter.texturedQuad(m_fallbackFbo->texture(), m_fallbackFbo->size(),
                                           *state->projectionMatrix(), *matrix(),
                                           inheritedOpacity());
        }
    }

    // Stroke!
    if (hasStroke) {
        const int strokeStencilValue = 0x80;
        const int writeMask = 0x80;

        setupStencilForCover(stencilClip, sv);
        if (stencilClip) {
            // for the stencil step (eff. read mask == 0xFF & ~writeMask)
            nvpr.pathStencilFunc(GL_EQUAL, sv, 0xFF);
            // With stencilCLip == true the read mask for the stencil test before the stencil step is 0x7F.
            // This assumes the clip stencil value is <= 127.
            if (sv >= strokeStencilValue)
                qWarning("PathItem/NVPR: stencil clip ref value %d too large; expect rendering errors", sv);
        }

        renderStroke(strokeStencilValue, writeMask);
    }

    if (stencilClip)
        nvpr.pathStencilFunc(GL_ALWAYS, 0, ~0);

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

bool QQuickNvprBlitter::create()
{
    if (isCreated())
        destroy();

    m_program = new QOpenGLShaderProgram;
    if (QOpenGLContext::currentContext()->format().profile() == QSurfaceFormat::CoreProfile) {
        m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/qt-project.org/items/shaders/shadereffect_core.vert"));
        m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/items/shaders/shadereffect_core.frag"));
    } else {
        m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/qt-project.org/items/shaders/shadereffect.vert"));
        m_program->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/items/shaders/shadereffect.frag"));
    }
    m_program->bindAttributeLocation("qt_Vertex", 0);
    m_program->bindAttributeLocation("qt_MultiTexCoord0", 1);
    if (!m_program->link())
        return false;

    m_matrixLoc = m_program->uniformLocation("qt_Matrix");
    m_opacityLoc = m_program->uniformLocation("qt_Opacity");

    m_buffer = new QOpenGLBuffer;
    if (!m_buffer->create())
        return false;
    m_buffer->bind();
    m_buffer->allocate(4 * sizeof(GLfloat) * 6);
    m_buffer->release();

    return true;
}

void QQuickNvprBlitter::destroy()
{
    if (m_program) {
        delete m_program;
        m_program = nullptr;
    }
    if (m_buffer) {
        delete m_buffer;
        m_buffer = nullptr;
    }
}

void QQuickNvprBlitter::texturedQuad(GLuint textureId, const QSize &size,
                                     const QMatrix4x4 &proj, const QMatrix4x4 &modelview,
                                     float opacity)
{
    QOpenGLExtraFunctions *f = QOpenGLContext::currentContext()->extraFunctions();

    m_program->bind();

    QMatrix4x4 m = proj * modelview;
    m_program->setUniformValue(m_matrixLoc, m);
    m_program->setUniformValue(m_opacityLoc, opacity);

    m_buffer->bind();

    if (size != m_prevSize) {
        m_prevSize = size;

        QPointF p0(size.width() - 1, size.height() - 1);
        QPointF p1(0, 0);
        QPointF p2(0, size.height() - 1);
        QPointF p3(size.width() - 1, 0);

        GLfloat vertices[6 * 4] = {
            GLfloat(p0.x()), GLfloat(p0.y()), 1, 0,
            GLfloat(p1.x()), GLfloat(p1.y()), 0, 1,
            GLfloat(p2.x()), GLfloat(p2.y()), 0, 0,

            GLfloat(p0.x()), GLfloat(p0.y()), 1, 0,
            GLfloat(p3.x()), GLfloat(p3.y()), 1, 1,
            GLfloat(p1.x()), GLfloat(p1.y()), 0, 1,
        };

        m_buffer->write(0, vertices, sizeof(vertices));
    }

    m_program->enableAttributeArray(0);
    m_program->enableAttributeArray(1);
    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const void *) (2 * sizeof(GLfloat)));

    f->glBindTexture(GL_TEXTURE_2D, textureId);

    f->glDrawArrays(GL_TRIANGLES, 0, 6);

    f->glBindTexture(GL_TEXTURE_2D, 0);
    m_buffer->release();
    m_program->release();
}

QT_END_NAMESPACE
