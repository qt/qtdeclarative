// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qsginternaltextnode_p.h"

#include "qquicktextnodeengine_p.h"

#include <private/qsgadaptationlayer_p.h>
#include <private/qsgdistancefieldglyphnode_p.h>
#include <private/qquickclipnode_p.h>
#include <private/qquickitem_p.h>
#include <private/qquicktextdocument_p.h>
#include <private/qsgrendernode_p.h>

#include <QtCore/qpoint.h>
#include <qtextdocument.h>
#include <qtextlayout.h>
#include <qabstracttextdocumentlayout.h>
#include <private/qquickstyledtext_p.h>
#include <private/qquicktext_p_p.h>
#include <private/qfont_p.h>
#include <private/qfontengine_p.h>

#include <private/qtextdocumentlayout_p.h>
#include <qhash.h>

// #define QSGINTERNALTEXTNODE_NO_RECYCLE

Q_STATIC_LOGGING_CATEGORY(lcTextRecycle, "qt.quick.text.noderecycling")

QT_BEGIN_NAMESPACE

/*!
    \class QSGInternalTextNode
    \inmodule QtQuick
    \internal

    Creates an empty QSGInternalTextNode
*/
QSGInternalTextNode::QSGInternalTextNode(QSGRenderContext *renderContext)
    : m_renderContext(renderContext)
{
#ifdef QSG_RUNTIME_DESCRIPTION
    qsgnode_set_description(this, QLatin1String("text"));
#endif

    static_assert(int(QSGTextNode::Normal) == int(QQuickText::Normal));
    static_assert(int(QSGTextNode::Outline) == int(QQuickText::Outline));
    static_assert(int(QSGTextNode::Raised) == int(QQuickText::Raised));
    static_assert(int(QSGTextNode::Sunken) == int(QQuickText::Sunken));

    static_assert(int(QSGTextNode::QtRendering) == int(QQuickText::QtRendering));
    static_assert(int(QSGTextNode::NativeRendering) == int(QQuickText::NativeRendering));
    static_assert(int(QSGTextNode::CurveRendering) == int(QQuickText::CurveRendering));
}

QSGInternalTextNode::~QSGInternalTextNode()
{
    qDeleteAll(m_textures);
}

QSGGlyphNode *QSGInternalTextNode::addGlyphs(const QPointF &position,
                                             const QGlyphRun &glyphs,
                                             const QColor &color,
                                             RecycleBin *recycleBin,
                                             QQuickText::TextStyle style,
                                             const QColor &styleColor,
                                             QSGNode *parentNode)
{
    QRawFont font = glyphs.rawFont();

    QSGTextNode::RenderType preferredRenderType = m_renderType;
    if (m_renderType != NativeRendering) {
        if (const QFontEngine *fe = QRawFontPrivate::get(font)->fontEngine)
            if (fe->hasUnreliableGlyphOutline() || !fe->isSmoothlyScalable)
                preferredRenderType = QSGTextNode::NativeRendering;
    }

    if (preferredRenderType == NativeRendering)
        m_containsUnscalableGlyphs = true;

    if (parentNode == nullptr)
        parentNode = this;

    QSGGlyphNode *node = findOrCreateGlyphNode(preferredRenderType, recycleBin, parentNode);
    node->setRenderTypeQuality(m_renderTypeQuality);
    node->setGlyphs(position + QPointF(0, glyphs.rawFont().ascent()), glyphs);
    node->setStyle(style);
    node->setStyleColor(styleColor);
    node->setColor(color);
    node->update();

    /* We flag the geometry as static, but we never call markVertexDataDirty
       or markIndexDataDirty on them. This is because all text nodes are
       discarded when a change occurs. If we start appending/removing from
       existing geometry, then we also need to start marking the geometry as
       dirty.
     */
    node->geometry()->setIndexDataPattern(QSGGeometry::StaticPattern);
    node->geometry()->setVertexDataPattern(QSGGeometry::StaticPattern);

    if (node->parent() == nullptr)
        parentNode->appendChildNode(node);

    if (style == QQuickText::Outline && color.alpha() > 0 && styleColor != color) {
        QSGGlyphNode *fillNode = findOrCreateGlyphNode(preferredRenderType, recycleBin, parentNode);
        fillNode->setRenderTypeQuality(m_renderTypeQuality);
        fillNode->setGlyphs(position + QPointF(0, glyphs.rawFont().ascent()), glyphs);
        fillNode->setStyle(QQuickText::Normal);
        fillNode->setPreferredAntialiasingMode(QSGGlyphNode::GrayAntialiasing);
        fillNode->setColor(color);
        fillNode->update();

        fillNode->geometry()->setIndexDataPattern(QSGGeometry::StaticPattern);
        fillNode->geometry()->setVertexDataPattern(QSGGeometry::StaticPattern);

        if (fillNode->parent() == nullptr)
            parentNode->appendChildNode(fillNode);
        fillNode->setRenderOrder(node->renderOrder() + 1);
    }

    return node;
}

void QSGInternalTextNode::setCursor(const QRectF &rect, const QColor &color, RecycleBin *recycleBin)
{
    if (m_cursorNode == nullptr)
        m_cursorNode = findOrCreateRectangleNode(recycleBin);
    m_cursorNode->setRect(rect);
    m_cursorNode->setColor(color);
    m_cursorNode->update();
    if (m_cursorNode->parent() == nullptr)
        appendChildNode(m_cursorNode);
}

void QSGInternalTextNode::clearCursor()
{
    if (m_cursorNode)
        removeChildNode(m_cursorNode);
    delete m_cursorNode;
    m_cursorNode = nullptr;
}

void QSGInternalTextNode::addDecorationNode(const QRectF &rect,
                                            const QColor &color,
                                            QTextCharFormat::UnderlineStyle style,
                                            RecycleBin *recycleBin)
{
    Q_UNUSED(style); // Software renderer does not support styled underlines
    addRectangleNode(rect, color, recycleBin);
}

void QSGInternalTextNode::addRectangleNode(const QRectF &rect,
                                           const QColor &color,
                                           RecycleBin *recycleBin)
{
    QSGInternalRectangleNode *node = findOrCreateRectangleNode(recycleBin);
    node->setRect(rect);
    node->setColor(color);
    node->update();

    if (node->parent() == nullptr)
        appendChildNode(node);
}

void QSGInternalTextNode::addImage(const QRectF &rect, const QImage &image, RecycleBin *recycleBin)
{
    QSGInternalImageNode *node = findOrCreateImageNode(recycleBin);
    QSGTexture *texture = m_renderContext->createTexture(image);
    texture->setFiltering(m_filtering);
    m_textures.append(texture);
    node->setTargetRect(rect);
    node->setInnerTargetRect(rect);
    node->setTexture(texture);
    node->setFiltering(m_filtering);
    if (node->parent() == nullptr)
        appendChildNode(node);
    node->update();
}

void QSGInternalTextNode::doAddTextDocument(QPointF position,
                                            QTextDocument *textDocument,
                                            int selectionStart,
                                            int selectionEnd,
                                            RecycleBin *recycleBin)
{
    QQuickTextNodeEngine engine;
    engine.setTextColor(m_color);
    engine.setSelectedTextColor(m_selectionTextColor);
    engine.setSelectionColor(m_selectionColor);
    engine.setAnchorColor(m_linkColor);
    engine.setPosition(position);
    engine.setDevicePixelRatio(m_devicePixelRatio);

    QList<QTextFrame *> frames;
    frames.append(textDocument->rootFrame());
    while (!frames.isEmpty()) {
        QTextFrame *textFrame = frames.takeFirst();
        frames.append(textFrame->childFrames());

        engine.addFrameDecorations(textDocument, textFrame);

        if (textFrame->firstPosition() > textFrame->lastPosition()
         && textFrame->frameFormat().position() != QTextFrameFormat::InFlow) {
            const int pos = textFrame->firstPosition() - 1;
            auto *a = static_cast<QtPrivate::ProtectedLayoutAccessor *>(textDocument->documentLayout());
            QTextCharFormat format = a->formatAccessor(pos);
            QRectF rect = a->frameBoundingRect(textFrame);

            QTextBlock block = textFrame->firstCursorPosition().block();
            engine.setCurrentLine(block.layout()->lineForTextPosition(pos - block.position()));
            engine.addTextObject(block, rect.topLeft(), format, QQuickTextNodeEngine::Unselected, textDocument,
                                 pos, textFrame->frameFormat().position());
        } else {
            QTextFrame::iterator it = textFrame->begin();

            while (!it.atEnd()) {
                Q_ASSERT(!engine.currentLine().isValid());

                QTextBlock block = it.currentBlock();
                engine.addTextBlock(textDocument, block, position, m_color, m_linkColor, selectionStart, selectionEnd,
                                    (textDocument->characterCount() > QQuickTextPrivate::largeTextSizeThreshold ?
                                         m_viewport : QRectF()));
                ++it;
            }
        }
    }

    engine.addToSceneGraph(this, recycleBin, QQuickText::TextStyle(m_textStyle), m_styleColor);
}

void QSGInternalTextNode::doAddTextLayout(QPointF position,
                                          QTextLayout *textLayout,
                                          int selectionStart,
                                          int selectionEnd,
                                          int lineStart,
                                          int lineCount,
                                          RecycleBin *recycleBin)
{
    QQuickTextNodeEngine engine;
    engine.setTextColor(m_color);
    engine.setSelectedTextColor(m_selectionTextColor);
    engine.setSelectionColor(m_selectionColor);
    engine.setAnchorColor(m_linkColor);
    engine.setPosition(position);
    engine.setDevicePixelRatio(m_devicePixelRatio);

#if QT_CONFIG(im)
    int preeditLength = textLayout->preeditAreaText().size();
    int preeditPosition = textLayout->preeditAreaPosition();
#endif

    QVarLengthArray<QTextLayout::FormatRange> colorChanges;
    engine.mergeFormats(textLayout, &colorChanges);

    lineCount = lineCount >= 0
            ? qMin(lineStart + lineCount, textLayout->lineCount())
            : textLayout->lineCount();

    bool inViewport = false;
    for (int i=lineStart; i<lineCount; ++i) {
        QTextLine line = textLayout->lineAt(i);

        int start = line.textStart();
        int length = line.textLength();
        int end = start + length;

#if QT_CONFIG(im)
        if (preeditPosition >= 0
         && preeditPosition >= start
         && preeditPosition < end) {
            end += preeditLength;
        }
#endif
        // If there's a lot of text, insert only the range of lines that can possibly be visible within the viewport.
        if (m_viewport.isNull() || (line.y() + line.height() > m_viewport.top() && line.y() < m_viewport.bottom())) {
            if (!inViewport && !m_viewport.isNull()) {
                m_firstLineInViewport = i;
                qCDebug(lcVP) << "first line in viewport" << i << "@" << line.y();
            }
            inViewport = true;
            engine.setCurrentLine(line);
            engine.addGlyphsForRanges(colorChanges, start, end, selectionStart, selectionEnd);
        } else if (inViewport) {
            Q_ASSERT(!m_viewport.isNull());
            m_firstLinePastViewport = i;
            qCDebug(lcVP) << "first omitted line past bottom of viewport" << i << "@" << line.y();
            break; // went past the bottom of the viewport, so we're done
        }
    }

    engine.addToSceneGraph(this, recycleBin, QQuickText::TextStyle(m_textStyle), m_styleColor);
}

namespace {

    using RecycleBin = QSGInternalTextNode::RecycleBin;

    class ChildNodeCollector : public QSGNodeVisitorEx
    {
    public:
        ChildNodeCollector(RecycleBin &recycleBin,
                           std::vector<QSGNode *> &untrackedNodes)
            : m_recycleBin(recycleBin)
            , m_untrackedNodes(untrackedNodes)
        {
        }

        bool visit(QSGTransformNode *node) override
        {
            Q_UNREACHABLE();
            m_untrackedNodes.push_back(node);
            return false;
        }

        void endVisit(QSGTransformNode *) override
        {
        }

        bool visit(QSGClipNode *node) override
        {
            // Clip nodes are used for selections and are currently not recycled. Clip nodes
            // will anyway be separate batch roots, so this text does not trigger reuploads.
            m_untrackedNodes.push_back(node);
            return false;
        }

        void endVisit(QSGClipNode *) override
        {
        }

        bool visit(QSGGeometryNode *node) override
        {
            Q_UNREACHABLE();
            m_untrackedNodes.push_back(node);
            return false;
        }

        void endVisit(QSGGeometryNode *) override
        {
        }

        bool visit(QSGOpacityNode *node) override
        {
            Q_UNREACHABLE();
            m_untrackedNodes.push_back(node);
            return false;
        }

        void endVisit(QSGOpacityNode *) override
        {
        }

        bool visit(QSGInternalImageNode *node) override
        {
            m_recycleBin.unusedNodes.append({ node, RecycleBin::ImageNode });
            return false;
        }

        void endVisit(QSGInternalImageNode *) override
        {
        }

        bool visit(QSGPainterNode *node) override
        {
            Q_UNREACHABLE();
            m_untrackedNodes.push_back(node);
            return false;
        }

        void endVisit(QSGPainterNode *) override
        {
        }

        bool visit(QSGInternalRectangleNode *node) override
        {
            m_recycleBin.unusedNodes.append({ node, RecycleBin::RectangleNode });
            return false;
        }

        void endVisit(QSGInternalRectangleNode *) override
        {
        }

        bool visit(QSGGlyphNode *node) override
        {
            // Some glyph nodes with a lot of content have content split into child nodes.
            // For now, we do not recycle these nodes as it is an uncommon case and adds
            // complexity. This also affects curve-rendered glyphs, which will have children
            // and thus be excluded from recycling.
            if (node->childCount() > 0)
                m_untrackedNodes.push_back(node);
            else
                m_recycleBin.unusedNodes.append({ node, RecycleBin::GlyphNode });

            return false;
        }

        void endVisit(QSGGlyphNode *) override
        {
        }

        bool visit(QSGRootNode *node) override
        {
            Q_UNREACHABLE();
            m_untrackedNodes.push_back(node);
            return false;
        }

        void endVisit(QSGRootNode *) override
        {
        }

#if QT_CONFIG(quick_sprite)
        bool visit(QSGSpriteNode *node) override
        {
            Q_UNREACHABLE();
            m_untrackedNodes.push_back(node);
            return false;
        }

        void endVisit(QSGSpriteNode *) override
        {
        }
#endif
        bool visit(QSGRenderNode *node) override
        {
            Q_UNREACHABLE();
            m_untrackedNodes.push_back(node);
            return false;
        }

        void endVisit(QSGRenderNode *) override
        {
        }

    private:
        RecycleBin &m_recycleBin;
        std::vector<QSGNode *> &m_untrackedNodes;
    };
}

QSGGlyphNode *QSGInternalTextNode::RecycleBin::takeNextReusableGlyphNode(
        QSGTextNode::RenderType renderType)
{
    const UnusedNode *next = peekNextReusableNode(GlyphNode);
    if (next == nullptr)
        return nullptr;

    QSGGlyphNode *glyphNode = static_cast<QSGGlyphNode *>(next->node);
    if (glyphNode->renderType() != renderType) {
        qCDebug(lcTextRecycle) << "    Unsuitable node found:" << glyphNode->renderType();
        stopReusing();
        return nullptr;
    }

    ++reusedNodes;
    return glyphNode;
}

QSGInternalRectangleNode *QSGInternalTextNode::findOrCreateRectangleNode(RecycleBin *recycleBin)
{
    qCDebug(lcTextRecycle) << "Searching for rectangle node";

    if (recycleBin != nullptr) {
        if (QSGVisitableNode *node = recycleBin->takeNextReusableNode(RecycleBin::RectangleNode)) {
            qCDebug(lcTextRecycle) << "    Found node, recycling";
            return static_cast<QSGInternalRectangleNode *>(node);
        }
    }

    qCDebug(lcTextRecycle) << "    Creating new rectangle node, no suitable node found";
    return m_renderContext->sceneGraphContext()->createInternalRectangleNode();
}


QSGInternalImageNode *QSGInternalTextNode::findOrCreateImageNode(RecycleBin *recycleBin)
{
    qCDebug(lcTextRecycle) << "Searching for image node";

    if (recycleBin != nullptr) {
        if (QSGVisitableNode *node = recycleBin->takeNextReusableNode(RecycleBin::ImageNode)) {
            qCDebug(lcTextRecycle) << "    Found node, recycling";
            return static_cast<QSGInternalImageNode *>(node);
        }
    }

    qCDebug(lcTextRecycle) << "    Creating new image node, no suitable node found";
    return m_renderContext->sceneGraphContext()->createInternalImageNode(m_renderContext);
}

QSGGlyphNode *QSGInternalTextNode::findOrCreateGlyphNode(RenderType renderType,
                                                         RecycleBin *recycleBin,
                                                         QSGNode *parentNode)
{
    qCDebug(lcTextRecycle) << "Searching for glyph node with renderType" << renderType;
    renderType = m_renderContext->sceneGraphContext()->processTextRenderType(renderType);

    if (recycleBin != nullptr) {
        if (parentNode != this) {
            recycleBin->stopReusing();
        } else if (QSGGlyphNode *node = recycleBin->takeNextReusableGlyphNode(renderType)) {
            qCDebug(lcTextRecycle) << "    Found node, recycling";
            return node;
        }
    }

    qCDebug(lcTextRecycle) << "    Creating new glyph node, no suitable node found";
    return m_renderContext->sceneGraphContext()->createGlyphNode(m_renderContext,
                                                                 renderType);
}

void QSGInternalTextNode::clear()
{
    while (firstChild() != nullptr)
        delete firstChild();
    m_cursorNode = nullptr;
    qDeleteAll(m_textures);
    m_textures.clear();
}

void QSGInternalTextNode::recycle(RecycleBin *recycleBin)
{
    Q_ASSERT(recycleBin != nullptr);
    Q_ASSERT(recycleBin->unusedNodes.isEmpty());

    recycleBin->unusedNodes.reserve(childCount());

    std::vector<QSGNode *> untrackedNodes;
    ChildNodeCollector collector(*recycleBin, untrackedNodes);

    // We retain nodes for recycling and then discard the unused ones after adding a new layout.
    // This is because removing and adding nodes to the root batch can cause expensive reuploads
    // of all data. So we visit all immediate children of the text node and keep them around
    // until they are either used or discardUnusedNodes() is called later. For rapid updates,
    // you will typically just have a single glyph node and rapidly replace its text, so this
    // is what we are optimzing for.
    QSGNode *node = firstChild();
    while (node != nullptr) {
        if (node->type() == QSGNode::GeometryNodeType
            && node->flags().testFlag(QSGNode::IsVisitableNode)) {
            QSGVisitableNode *visitableNode = static_cast<QSGVisitableNode *>(node);
            visitableNode->accept(&collector);
        } else {
            untrackedNodes.push_back(node);
        }

        node = node->nextSibling();
    }

    for (const RecycleBin::UnusedNode &unusedNode : recycleBin->unusedNodes) {
        if (unusedNode.type == RecycleBin::GlyphNode)
            static_cast<QSGGlyphNode *>(unusedNode.node)->recycle();
    }

    qDeleteAll(untrackedNodes);

#if defined(QSGINTERNALTEXTNODE_NO_RECYCLE)
    for (const RecycleBin::UnusedNode &unusedNode : recycleBin->unusedNodes)
        delete unusedNode.node;
    recycleBin->unusedNodes = {};

    qDeleteAll(m_textures);
#else
    recycleBin->unusedTextures = m_textures;
#endif

    qCDebug(lcTextRecycle) << "Recycle: "
                           << recycleBin->unusedNodes.size() << "nodes"
                           << recycleBin->unusedTextures.size() << "textures";

    m_textures.clear();
    m_cursorNode = nullptr;
    recycleBin->unusedNodes.squeeze();
}

void QSGInternalTextNode::discardUnusedNodes(RecycleBin *recycleBin)
{
    qCDebug(lcTextRecycle) << "Discard: "
                           << recycleBin->unusedNodes.size() - recycleBin->reusedNodes << "nodes"
                           << recycleBin->unusedTextures.size() << "textures";

    for (qsizetype i = recycleBin->reusedNodes; i < recycleBin->unusedNodes.size(); ++i)
        delete recycleBin->unusedNodes.at(i).node;
    recycleBin->unusedNodes = {};
    recycleBin->reusedNodes = 0;
    recycleBin->reuseStopped = false;

    qDeleteAll(recycleBin->unusedTextures);
    recycleBin->unusedTextures = {};
}

QT_END_NAMESPACE
