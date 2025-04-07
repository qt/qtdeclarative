// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsginternaltextnode_p.h"

#include "qquicktextnodeengine_p.h"

#include <private/qsgadaptationlayer_p.h>
#include <private/qsgdistancefieldglyphnode_p.h>
#include <private/qquickclipnode_p.h>
#include <private/qquickitem_p.h>
#include <private/qquicktextdocument_p.h>

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

QT_BEGIN_NAMESPACE

/*!
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

QSGGlyphNode *QSGInternalTextNode::addGlyphs(const QPointF &position, const QGlyphRun &glyphs, const QColor &color,
                                             QQuickText::TextStyle style, const QColor &styleColor,
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

    QSGGlyphNode *node = m_renderContext->sceneGraphContext()->createGlyphNode(m_renderContext,
                                                                               preferredRenderType,
                                                                               m_renderTypeQuality);
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

    if (parentNode == nullptr)
        parentNode = this;
    parentNode->appendChildNode(node);

    if (style == QQuickText::Outline && color.alpha() > 0 && styleColor != color) {
        QSGGlyphNode *fillNode = m_renderContext->sceneGraphContext()->createGlyphNode(m_renderContext,
                                                                                       preferredRenderType,
                                                                                       m_renderTypeQuality);
        fillNode->setGlyphs(position + QPointF(0, glyphs.rawFont().ascent()), glyphs);
        fillNode->setStyle(QQuickText::Normal);
        fillNode->setPreferredAntialiasingMode(QSGGlyphNode::GrayAntialiasing);
        fillNode->setColor(color);
        fillNode->update();

        fillNode->geometry()->setIndexDataPattern(QSGGeometry::StaticPattern);
        fillNode->geometry()->setVertexDataPattern(QSGGeometry::StaticPattern);

        parentNode->appendChildNode(fillNode);
        fillNode->setRenderOrder(node->renderOrder() + 1);
    }

    return node;
}

void QSGInternalTextNode::setCursor(const QRectF &rect, const QColor &color)
{
    if (m_cursorNode != nullptr)
        delete m_cursorNode;

    m_cursorNode = m_renderContext->sceneGraphContext()->createInternalRectangleNode(rect, color);
    appendChildNode(m_cursorNode);
}

void QSGInternalTextNode::clearCursor()
{
    if (m_cursorNode)
        removeChildNode(m_cursorNode);
    delete m_cursorNode;
    m_cursorNode = nullptr;
}

void QSGInternalTextNode::addDecorationNode(const QRectF &rect, const QColor &color)
{
    addRectangleNode(rect, color);
}

void QSGInternalTextNode::addRectangleNode(const QRectF &rect, const QColor &color)
{
    appendChildNode(m_renderContext->sceneGraphContext()->createInternalRectangleNode(rect, color));
}

void QSGInternalTextNode::addImage(const QRectF &rect, const QImage &image)
{
    QSGInternalImageNode *node = m_renderContext->sceneGraphContext()->createInternalImageNode(m_renderContext);
    QSGTexture *texture = m_renderContext->createTexture(image);
    texture->setFiltering(m_filtering);
    m_textures.append(texture);
    node->setTargetRect(rect);
    node->setInnerTargetRect(rect);
    node->setTexture(texture);
    node->setFiltering(m_filtering);
    appendChildNode(node);
    node->update();
}

void QSGInternalTextNode::doAddTextDocument(QPointF position, QTextDocument *textDocument,
                                            int selectionStart, int selectionEnd)
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

    engine.addToSceneGraph(this, QQuickText::TextStyle(m_textStyle), m_styleColor);
}

void QSGInternalTextNode::doAddTextLayout(QPointF position, QTextLayout *textLayout,
                                          int selectionStart, int selectionEnd,
                                          int lineStart, int lineCount)
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

    engine.addToSceneGraph(this, QQuickText::TextStyle(m_textStyle), m_styleColor);
}

void QSGInternalTextNode::clear()
{
    while (firstChild() != nullptr)
        delete firstChild();
    m_cursorNode = nullptr;
    qDeleteAll(m_textures);
    m_textures.clear();
}

QT_END_NAMESPACE
