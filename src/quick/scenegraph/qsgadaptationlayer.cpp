/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgadaptationlayer_p.h"

#include <qmath.h>
#include <QtQuick/private/qsgdistancefieldutil_p.h>
#include <QtQuick/private/qsgdistancefieldglyphnode_p.h>
#include <QtQuick/private/qsgcontext_p.h>
#include <private/qrawfont_p.h>
#include <QtGui/qguiapplication.h>
#include <qdir.h>

#include <private/qquickprofiler_p.h>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE

static QElapsedTimer qsg_render_timer;

QSGDistanceFieldGlyphCache::Texture QSGDistanceFieldGlyphCache::s_emptyTexture;

QSGDistanceFieldGlyphCache::QSGDistanceFieldGlyphCache(QSGDistanceFieldGlyphCacheManager *man, QOpenGLContext *c, const QRawFont &font)
    : m_manager(man)
    , m_pendingGlyphs(64)
{
    Q_ASSERT(font.isValid());

    QRawFontPrivate *fontD = QRawFontPrivate::get(font);
    m_glyphCount = fontD->fontEngine->glyphCount();

    m_doubleGlyphResolution = qt_fontHasNarrowOutlines(font) && m_glyphCount < QT_DISTANCEFIELD_HIGHGLYPHCOUNT;

    m_referenceFont = font;
    m_referenceFont.setPixelSize(QT_DISTANCEFIELD_BASEFONTSIZE(m_doubleGlyphResolution));
    Q_ASSERT(m_referenceFont.isValid());

    m_coreProfile = (c->format().profile() == QSurfaceFormat::CoreProfile);
}

QSGDistanceFieldGlyphCache::~QSGDistanceFieldGlyphCache()
{
}

QSGDistanceFieldGlyphCache::GlyphData &QSGDistanceFieldGlyphCache::glyphData(glyph_t glyph)
{
    QHash<glyph_t, GlyphData>::iterator data = m_glyphsData.find(glyph);
    if (data == m_glyphsData.end()) {
        GlyphData gd;
        gd.texture = &s_emptyTexture;
        QPainterPath path = m_referenceFont.pathForGlyph(glyph);
        gd.boundingRect = path.boundingRect();
        data = m_glyphsData.insert(glyph, gd);
    }
    return data.value();
}

QSGDistanceFieldGlyphCache::Metrics QSGDistanceFieldGlyphCache::glyphMetrics(glyph_t glyph, qreal pixelSize)
{
    GlyphData &gd = glyphData(glyph);
    qreal scale = fontScale(pixelSize);

    Metrics m;
    m.width = gd.boundingRect.width() * scale;
    m.height = gd.boundingRect.height() * scale;
    m.baselineX = gd.boundingRect.x() * scale;
    m.baselineY = -gd.boundingRect.y() * scale;

    return m;
}

void QSGDistanceFieldGlyphCache::populate(const QVector<glyph_t> &glyphs)
{
    QSet<glyph_t> referencedGlyphs;
    QSet<glyph_t> newGlyphs;
    int count = glyphs.count();
    for (int i = 0; i < count; ++i) {
        glyph_t glyphIndex = glyphs.at(i);
        if ((int) glyphIndex >= glyphCount()) {
            qWarning("Warning: distance-field glyph is not available with index %d", glyphIndex);
            continue;
        }

        GlyphData &gd = glyphData(glyphIndex);
        ++gd.ref;
        referencedGlyphs.insert(glyphIndex);

        if (gd.texCoord.isValid() || m_populatingGlyphs.contains(glyphIndex))
            continue;

        m_populatingGlyphs.insert(glyphIndex);

        if (gd.boundingRect.isEmpty()) {
            gd.texCoord.width = 0;
            gd.texCoord.height = 0;
        } else {
            newGlyphs.insert(glyphIndex);
        }
    }

    referenceGlyphs(referencedGlyphs);
    if (!newGlyphs.isEmpty())
        requestGlyphs(newGlyphs);
}

void QSGDistanceFieldGlyphCache::release(const QVector<glyph_t> &glyphs)
{
    QSet<glyph_t> unusedGlyphs;
    int count = glyphs.count();
    for (int i = 0; i < count; ++i) {
        glyph_t glyphIndex = glyphs.at(i);
        GlyphData &gd = glyphData(glyphIndex);
        if (--gd.ref == 0 && !gd.texCoord.isNull())
            unusedGlyphs.insert(glyphIndex);
    }
    releaseGlyphs(unusedGlyphs);
}

void QSGDistanceFieldGlyphCache::update()
{
    m_populatingGlyphs.clear();

    if (m_pendingGlyphs.isEmpty())
        return;

    bool profileFrames = QSG_LOG_TIME_GLYPH().isDebugEnabled();
    if (profileFrames)
        qsg_render_timer.start();
    Q_QUICK_SG_PROFILE_START(QQuickProfiler::SceneGraphAdaptationLayerFrame);

    QList<QDistanceField> distanceFields;
    for (int i = 0; i < m_pendingGlyphs.size(); ++i) {
        distanceFields.append(QDistanceField(m_referenceFont,
                                             m_pendingGlyphs.at(i),
                                             m_doubleGlyphResolution));
    }

    qint64 renderTime = 0;
    int count = m_pendingGlyphs.size();
    if (profileFrames)
        renderTime = qsg_render_timer.nsecsElapsed();
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphAdaptationLayerFrame);

    m_pendingGlyphs.reset();

    storeGlyphs(distanceFields);

    if (QSG_LOG_TIME_GLYPH().isDebugEnabled()) {
        quint64 now = qsg_render_timer.elapsed();
        qCDebug(QSG_LOG_TIME_GLYPH,
                "distancefield: %d glyphs prepared in %dms, rendering=%d, upload=%d",
                count,
                (int) now,
                int(renderTime / 1000000),
                int((now - (renderTime / 1000000))));
    }
    Q_QUICK_SG_PROFILE_END_WITH_PAYLOAD(QQuickProfiler::SceneGraphAdaptationLayerFrame,
                                        (qint64)count);
}

void QSGDistanceFieldGlyphCache::setGlyphsPosition(const QList<GlyphPosition> &glyphs)
{
    QVector<quint32> invalidatedGlyphs;

    int count = glyphs.count();
    for (int i = 0; i < count; ++i) {
        GlyphPosition glyph = glyphs.at(i);
        GlyphData &gd = glyphData(glyph.glyph);

        if (!gd.texCoord.isNull())
            invalidatedGlyphs.append(glyph.glyph);

        gd.texCoord.xMargin = QT_DISTANCEFIELD_RADIUS(m_doubleGlyphResolution) / qreal(QT_DISTANCEFIELD_SCALE(m_doubleGlyphResolution));
        gd.texCoord.yMargin = QT_DISTANCEFIELD_RADIUS(m_doubleGlyphResolution) / qreal(QT_DISTANCEFIELD_SCALE(m_doubleGlyphResolution));
        gd.texCoord.x = glyph.position.x();
        gd.texCoord.y = glyph.position.y();
        gd.texCoord.width = gd.boundingRect.width();
        gd.texCoord.height = gd.boundingRect.height();
    }

    if (!invalidatedGlyphs.isEmpty()) {
        QLinkedList<QSGDistanceFieldGlyphConsumer *>::iterator it = m_registeredNodes.begin();
        while (it != m_registeredNodes.end()) {
            (*it)->invalidateGlyphs(invalidatedGlyphs);
            ++it;
        }
    }
}

void QSGDistanceFieldGlyphCache::registerOwnerElement(QQuickItem *ownerElement)
{
    Q_UNUSED(ownerElement);
}

void QSGDistanceFieldGlyphCache::unregisterOwnerElement(QQuickItem *ownerElement)
{
    Q_UNUSED(ownerElement);
}

void QSGDistanceFieldGlyphCache::processPendingGlyphs()
{
    /* Intentionally empty */
}

void QSGDistanceFieldGlyphCache::setGlyphsTexture(const QVector<glyph_t> &glyphs, const Texture &tex)
{
    int i = m_textures.indexOf(tex);
    if (i == -1) {
        m_textures.append(tex);
        i = m_textures.size() - 1;
    } else {
        m_textures[i].size = tex.size;
    }
    Texture *texture = &(m_textures[i]);

    QVector<quint32> invalidatedGlyphs;

    int count = glyphs.count();
    for (int j = 0; j < count; ++j) {
        glyph_t glyphIndex = glyphs.at(j);
        GlyphData &gd = glyphData(glyphIndex);
        if (gd.texture != &s_emptyTexture)
            invalidatedGlyphs.append(glyphIndex);
        gd.texture = texture;
    }

    if (!invalidatedGlyphs.isEmpty()) {
        QLinkedList<QSGDistanceFieldGlyphConsumer*>::iterator it = m_registeredNodes.begin();
        while (it != m_registeredNodes.end()) {
            (*it)->invalidateGlyphs(invalidatedGlyphs);
            ++it;
        }
    }
}

void QSGDistanceFieldGlyphCache::markGlyphsToRender(const QVector<glyph_t> &glyphs)
{
    int count = glyphs.count();
    for (int i = 0; i < count; ++i)
        m_pendingGlyphs.add(glyphs.at(i));
}

void QSGDistanceFieldGlyphCache::updateTexture(GLuint oldTex, GLuint newTex, const QSize &newTexSize)
{
    int count = m_textures.count();
    for (int i = 0; i < count; ++i) {
        Texture &tex = m_textures[i];
        if (tex.textureId == oldTex) {
            tex.textureId = newTex;
            tex.size = newTexSize;
            return;
        }
    }
}

void QSGNodeVisitorEx::visitChildren(QSGNode *node)
{
    for (QSGNode *child = node->firstChild(); child; child = child->nextSibling()) {
        switch (child->type()) {
        case QSGNode::ClipNodeType: {
            QSGClipNode *c = static_cast<QSGClipNode*>(child);
            if (visit(c))
                visitChildren(c);
            endVisit(c);
            break;
        }
        case QSGNode::TransformNodeType: {
            QSGTransformNode *c = static_cast<QSGTransformNode*>(child);
            if (visit(c))
                visitChildren(c);
            endVisit(c);
            break;
        }
        case QSGNode::OpacityNodeType: {
            QSGOpacityNode *c = static_cast<QSGOpacityNode*>(child);
            if (visit(c))
                visitChildren(c);
            endVisit(c);
            break;
        }
        case QSGNode::GeometryNodeType: {
            if (child->flags() & QSGNode::IsVisitableNode) {
                QSGVisitableNode *v = static_cast<QSGVisitableNode*>(child);
                v->accept(this);
            } else {
                QSGGeometryNode *c = static_cast<QSGGeometryNode*>(child);
                if (visit(c))
                    visitChildren(c);
                endVisit(c);
            }
            break;
        }
        case QSGNode::RootNodeType: {
            QSGRootNode *root = static_cast<QSGRootNode*>(child);
            if (visit(root))
                visitChildren(root);
            endVisit(root);
            break;
        }
        case QSGNode::BasicNodeType: {
            visitChildren(child);
            break;
        }
        default:
            Q_UNREACHABLE();
            break;
        }
    }
}

void QSGLayer::markDirtyTextureLater()
{
    QCoreApplication::postEvent(this, new QEvent(static_cast<QEvent::Type>(markDirtyEventType())));
}

void QSGLayer::customEvent(QEvent *event)
{
    if (event->type() == markDirtyEventType())
        markDirtyTexture();
    else
        QObject::customEvent(event);
}

int QSGLayer::markDirtyEventType()
{
    static int type = QEvent::None;
    if (type == QEvent::None)
        type = QEvent::registerEventType();
    return type;
}

QT_END_NAMESPACE
