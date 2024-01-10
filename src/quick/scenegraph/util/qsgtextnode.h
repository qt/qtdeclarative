// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGTEXTNODE_H
#define QSGTEXTNODE_H

#include <QtGui/qtextlayout.h>
#include <QtQuick/qsgnode.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGTextNode : public QSGTransformNode
{
public:
    // Should match the TextStyle in qquicktext_p.h
    enum TextStyle : quint8
    {
        Normal,
        Outline,
        Raised,
        Sunken
    };

    // Should match the RenderType in qquicktext_p.h
    enum RenderType: quint8
    {
        QtRendering,
        NativeRendering,
        CurveRendering
    };

    ~QSGTextNode() override = default;

    virtual void setColor(const QColor &color) = 0;
    virtual QColor color() const = 0;

    virtual void setTextStyle(TextStyle textStyle) = 0;
    virtual TextStyle textStyle() = 0;

    virtual void setStyleColor(const QColor &styleColor) = 0;
    virtual QColor styleColor() const = 0;

    virtual void setLinkColor(const QColor &linkColor) = 0;
    virtual QColor linkColor() const = 0;

    virtual void setSelectionColor(const QColor &selectionColor) = 0;
    virtual QColor selectionColor() const = 0;

    virtual void setSelectionTextColor(const QColor &selectionTextColor) = 0;
    virtual QColor selectionTextColor() const = 0;

    virtual void setRenderType(RenderType renderType) = 0;
    virtual RenderType renderType() const = 0;

    virtual void setRenderTypeQuality(int renderTypeQuality) = 0;
    virtual int renderTypeQuality() const = 0;

    virtual void setSmooth(bool smooth) = 0;
    virtual bool smooth() const = 0;

    virtual void clear() = 0;

    virtual void setViewport(const QRectF &viewport) = 0;
    virtual QRectF viewport() const = 0;

    virtual void addTextLayout(const QPointF &position,
                               QTextLayout *layout,
                               int selectionStart = -1,
                               int selectionCount = -1,
                               int lineStart = 0,
                               int lineCount = -1) = 0;
    virtual void addTextDocument(const QPointF &position,
                                 QTextDocument *document,
                                 int selectionStart = -1,
                                 int selectionCount = -1) = 0;

};

QT_END_NAMESPACE

#endif // QSGTEXTNODE_H
