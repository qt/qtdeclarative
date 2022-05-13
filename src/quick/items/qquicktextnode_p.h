// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKTEXTNODE_P_H
#define QQUICKTEXTNODE_P_H

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

#include <QtQuick/qsgnode.h>
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
class QSGInternalRectangleNode;
class QSGClipNode;
class QSGTexture;

class QQuickTextNodeEngine;

class Q_QUICK_PRIVATE_EXPORT QQuickTextNode : public QSGTransformNode
{
public:
    QQuickTextNode(QQuickItem *ownerElement);
    ~QQuickTextNode();

    static bool isComplexRichText(QTextDocument *);

    void deleteContent();
    void addTextLayout(const QPointF &position, QTextLayout *textLayout, const QColor &color = QColor(),
                       QQuickText::TextStyle style = QQuickText::Normal, const QColor &styleColor = QColor(),
                       const QColor &anchorColor = QColor(),
                       const QColor &selectionColor = QColor(), const QColor &selectedTextColor = QColor(),
                       int selectionStart = -1, int selectionEnd = -1,
                       int lineStart = 0, int lineCount = -1);
    void addTextDocument(const QPointF &position, QTextDocument *textDocument, const QColor &color = QColor(),
                         QQuickText::TextStyle style = QQuickText::Normal, const QColor &styleColor = QColor(),
                         const QColor &anchorColor = QColor(),
                         const QColor &selectionColor = QColor(), const QColor &selectedTextColor = QColor(),
                         int selectionStart = -1, int selectionEnd = -1);

    void setCursor(const QRectF &rect, const QColor &color);
    void clearCursor();
    QSGInternalRectangleNode *cursorNode() const { return m_cursorNode; }

    QSGGlyphNode *addGlyphs(const QPointF &position, const QGlyphRun &glyphs, const QColor &color,
                            QQuickText::TextStyle style = QQuickText::Normal, const QColor &styleColor = QColor(),
                            QSGNode *parentNode = 0);
    void addImage(const QRectF &rect, const QImage &image);
    void addRectangleNode(const QRectF &rect, const QColor &color);

    bool useNativeRenderer() const { return m_useNativeRenderer; }
    void setUseNativeRenderer(bool on) { m_useNativeRenderer = on; }

    void setRenderTypeQuality(int renderTypeQuality) { m_renderTypeQuality = renderTypeQuality; }
    int renderTypeQuality() const { return m_renderTypeQuality; }

    QPair<int, int> renderedLineRange() const { return { m_firstLineInViewport, m_firstLinePastViewport }; }

private:
    QSGInternalRectangleNode *m_cursorNode;
    QList<QSGTexture *> m_textures;
    QQuickItem *m_ownerElement;
    bool m_useNativeRenderer;
    int m_renderTypeQuality;
    int m_firstLineInViewport = -1;
    int m_firstLinePastViewport = -1;

    friend class QQuickTextEdit;
    friend class QQuickTextEditPrivate;
};

QT_END_NAMESPACE

#endif // QQUICKTEXTNODE_P_H
