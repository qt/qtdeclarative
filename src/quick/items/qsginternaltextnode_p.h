// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSGINTERNALTEXTNODE_P_H
#define QSGINTERNALTEXTNODE_P_H

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

#include "qsgtextnode.h"
#include "qquicktext_p.h"
#include <qglyphrun.h>

#include <QtGui/qcolor.h>
#include <QtGui/qtextlayout.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QSGGlyphNode;
class QTextBlock;
class QColor;
class QTextDocument;
class QSGContext;
class QRawFont;
class QSGInternalImageNode;
class QSGInternalRectangleNode;
class QSGClipNode;
class QSGTexture;
class QSGRenderContext;

class QQuickTextNodeEngine;

class Q_QUICK_EXPORT QSGInternalTextNode : public QSGTextNode
{
public:
    struct RecycleBin {
        RecycleBin() = default;
        ~RecycleBin()
        {
            Q_ASSERT(unusedGlyphNodes.isEmpty());
            Q_ASSERT(unusedRectangleNodes.isEmpty());
            Q_ASSERT(unusedImageNodes.isEmpty());
            Q_ASSERT(unusedTextures.isEmpty());
        }

        QVarLengthArray<QSGGlyphNode *, 8> unusedGlyphNodes;
        QVarLengthArray<QSGInternalRectangleNode *, 8> unusedRectangleNodes;
        QVarLengthArray<QSGInternalImageNode *, 8> unusedImageNodes;
        QList<QSGTexture *> unusedTextures;

    private:
        Q_DISABLE_COPY(RecycleBin)
    };

    QSGInternalTextNode(QSGRenderContext *renderContext);
    ~QSGInternalTextNode();

    static bool isComplexRichText(QTextDocument *);

    void setColor(QColor color) override
    {
        m_color = color;
    }

    QColor color() const override
    {
        return m_color;
    }

    void setTextStyle(TextStyle textStyle) override
    {
        m_textStyle = textStyle;
    }

    TextStyle textStyle() override
    {
        return m_textStyle;
    }

    void setStyleColor(QColor styleColor) override
    {
        m_styleColor = styleColor;
    }

    QColor styleColor() const override
    {
        return m_styleColor;
    }

    void setLinkColor(QColor linkColor) override
    {
        m_linkColor = linkColor;
    }

    QColor linkColor() const override
    {
        return m_linkColor;
    }

    void setSelectionColor(QColor selectionColor) override
    {
        m_selectionColor = selectionColor;
    }

    QColor selectionColor() const override
    {
        return m_selectionColor;
    }

    void setSelectionTextColor(QColor selectionTextColor) override
    {
        m_selectionTextColor = selectionTextColor;
    }

    QColor selectionTextColor() const override
    {
        return m_selectionTextColor;
    }

    void setRenderTypeQuality(int renderTypeQuality) override
    {
        m_renderTypeQuality = renderTypeQuality;
    }
    int renderTypeQuality() const override
    {
        return m_renderTypeQuality;
    }

    void setRenderType(RenderType renderType) override
    {
        m_renderType = renderType;
    }

    RenderType renderType() const override
    {
        return m_renderType;
    }

    bool containsUnscalableGlyphs() const
    {
        return m_containsUnscalableGlyphs;
    }

    void setFiltering(QSGTexture::Filtering filtering) override
    {
        m_filtering = filtering;
    }

    QSGTexture::Filtering filtering() const override
    {
        return m_filtering;
    }

    void setViewport(const QRectF &viewport) override
    {
        m_viewport = viewport;
    }

    QRectF viewport() const override
    {
        return m_viewport;
    }

    void setDevicePixelRatio(qreal dpr)
    {
        m_devicePixelRatio = dpr;
    }

    void setCursor(const QRectF &rect, const QColor &color, RecycleBin *recycleBin);
    void clearCursor();

    void addRectangleNode(const QRectF &rect, const QColor &color, RecycleBin *recycleBin);
    virtual void addDecorationNode(const QRectF &rect,
                                   const QColor &color,
                                   QTextCharFormat::UnderlineStyle style,
                                   RecycleBin *recycleBin);
    void addImage(const QRectF &rect, const QImage &image, RecycleBin *recycleBin);
    void clear() override;
    void recycle(RecycleBin *recycleBin);
    QSGGlyphNode *addGlyphs(const QPointF &position,
                            const QGlyphRun &glyphs,
                            const QColor &color,
                            RecycleBin *recycleBin,
                            QQuickText::TextStyle style = QQuickText::Normal,
                            const QColor &styleColor = QColor(),
                            QSGNode *parentNode = 0);

    QSGInternalRectangleNode *cursorNode() const { return m_cursorNode; }
    std::pair<int, int> renderedLineRange() const { return { m_firstLineInViewport, m_firstLinePastViewport }; }

    void discardUnusedNodes(RecycleBin *recycleBin);

    void addTextLayout(QPointF position,
                       QTextLayout *layout,
                       RecycleBin *recycleBin,
                       int selectionStart = -1,
                       int selectionCount = -1,
                       int lineStart = 0,
                       int lineCount = -1)
    {
        doAddTextLayout(position, layout, selectionStart, selectionCount, lineStart, lineCount, recycleBin);
    }

    void addTextDocument(QPointF position,
                         QTextDocument *document,
                         RecycleBin *recycleBin,
                         int selectionStart = -1,
                         int selectionCount = -1)
    {
        doAddTextDocument(position, document, selectionStart, selectionCount, recycleBin);
    }

protected:
    void doAddTextLayout(QPointF position,
                         QTextLayout *textLayout,
                         int selectionStart,
                         int selectionEnd,
                         int lineStart,
                         int lineCount,
                         RecycleBin *recycleBin);

    void doAddTextLayout(QPointF position,
                         QTextLayout *textLayout,
                         int selectionStart,
                         int selectionEnd,
                         int lineStart,
                         int lineCount) override
    {
        doAddTextLayout(position, textLayout, selectionStart, selectionEnd, lineStart, lineCount, nullptr);
    }

    void doAddTextDocument(QPointF position,
                           QTextDocument *textDocument,
                           int selectionStart,
                           int selectionEnd,
                           RecycleBin *recycleBin);
    void doAddTextDocument(QPointF position,
                           QTextDocument *textDocument,
                           int selectionStart,
                           int selectionEnd) override
    {
        doAddTextDocument(position, textDocument, selectionStart, selectionEnd, nullptr);
    }

private:
    QSGInternalImageNode *findOrCreateImageNode(RecycleBin *recycleBin);
    QSGGlyphNode *findOrCreateGlyphNode(RenderType renderType, RecycleBin *recycleBin);
    QSGInternalRectangleNode *findOrCreateRectangleNode(RecycleBin *recycleBin);

    QSGInternalRectangleNode *m_cursorNode = nullptr;
    QList<QSGTexture *> m_textures;
    QSGRenderContext *m_renderContext = nullptr;
    RenderType m_renderType = QtRendering;
    TextStyle m_textStyle = Normal;
    QRectF m_viewport;
    QColor m_color = QColor(0, 0, 0);
    QColor m_styleColor = QColor(0, 0, 0);
    QColor m_linkColor = QColor(0, 0, 255);
    QColor m_selectionColor = QColor(0, 0, 128);
    QColor m_selectionTextColor = QColor(255, 255, 255);
    QSGTexture::Filtering m_filtering = QSGTexture::Nearest;
    int m_renderTypeQuality = -1;
    int m_firstLineInViewport = -1;
    int m_firstLinePastViewport = -1;
    bool m_containsUnscalableGlyphs = false;
    qreal m_devicePixelRatio = 1.0;

    friend class QQuickTextEdit;
    friend class QQuickTextEditPrivate;
};

QT_END_NAMESPACE

#endif // QSGINTERNALTEXTNODE_P_H
