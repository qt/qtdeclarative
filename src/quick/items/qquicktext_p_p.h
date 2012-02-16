/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKTEXT_P_P_H
#define QQUICKTEXT_P_P_H

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

#include "qquicktext_p.h"
#include "qquickimplicitsizeitem_p_p.h"

#include <QtQml/qqml.h>
#include <QtGui/qabstracttextdocumentlayout.h>
#include <QtGui/qtextlayout.h>
#include <private/qquickstyledtext_p.h>

QT_BEGIN_NAMESPACE

class QTextLayout;
class QQuickTextDocumentWithImageResources;

class Q_AUTOTEST_EXPORT QQuickTextPrivate : public QQuickImplicitSizeItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickText)
public:
    QQuickTextPrivate();
    ~QQuickTextPrivate();
    void init();

    void updateSize();
    void updateLayout();
    bool determineHorizontalAlignment();
    bool setHAlign(QQuickText::HAlignment, bool forceAlign = false);
    void mirrorChange();
    QTextDocument *textDocument();
    bool isLineLaidOutConnected();
    void setLineGeometry(QTextLine &line, qreal lineWidth, qreal &height);
    QString elidedText(int lineWidth, const QTextLine &line, QTextLine *nextLine = 0) const;

    QRect layedOutTextRect;

    qreal lineHeight;

    QString text;
    QString activeLink;
    QUrl baseUrl;
    QFont font;
    QFont sourceFont;
    QList<QQuickStyledTextImgTag*> imgTags;
    QList<QQuickStyledTextImgTag*> visibleImgTags;

    QTextLayout layout;
    QTextLayout *elideLayout;
    QQuickTextLine *textLine;
    QQuickTextDocumentWithImageResources *doc;

#if defined(Q_OS_MAC)
    QList<QRectF> linesRects;
    QThread *layoutThread;
    QThread *paintingThread;
#endif

    QRgb color;
    QRgb linkColor;
    QRgb styleColor;

    int lineCount;
    int maximumLineCount;
    int multilengthEos;
    int minimumPixelSize;
    int minimumPointSize;
    int nbActiveDownloads;

    enum UpdateType {
        UpdateNone,
        UpdatePreprocess,
        UpdatePaintNode
    };

    QQuickText::HAlignment hAlign;
    QQuickText::VAlignment vAlign;
    QQuickText::TextElideMode elideMode;
    QQuickText::TextFormat format;
    QQuickText::WrapMode wrapMode;
    QQuickText::LineHeightMode lineHeightMode;
    QQuickText::TextStyle style;
    QQuickText::FontSizeMode fontSizeMode;
    UpdateType updateType;

    bool maximumLineCountValid:1;
    bool updateOnComponentComplete:1;
    bool updateLayoutOnPolish:1;
    bool richText:1;
    bool styledText:1;
    bool singleline:1;
    bool internalWidthUpdate:1;
    bool requireImplicitWidth:1;
    bool truncated:1;
    bool hAlignImplicit:1;
    bool rightToLeftText:1;
    bool layoutTextElided:1;
    bool textHasChanged:1;
    bool needToUpdateLayout:1;

    static const QChar elideChar;

    virtual qreal getImplicitWidth() const;

    void ensureDoc();

    QRect setupTextLayout(qreal *const naturalWidth);
    void setupCustomLineGeometry(QTextLine &line, qreal &height, int lineOffset = 0);
    bool isLinkActivatedConnected();
    QString anchorAt(const QPointF &pos);

    static inline QQuickTextPrivate *get(QQuickText *t) {
        return t->d_func();
    }
};

class QQuickPixmap;
class QQuickTextDocumentWithImageResources : public QTextDocument, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
public:
    QQuickTextDocumentWithImageResources(QQuickItem *parent);
    virtual ~QQuickTextDocumentWithImageResources();

    void setText(const QString &);
    int resourcesLoading() const { return outstanding; }

    void clearResources();

    void clear();

    QSizeF intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format);
    void drawObject(QPainter *p, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format);

    QImage image(const QTextImageFormat &format);

    void setBaseUrl(const QUrl &url, bool clear = true);

Q_SIGNALS:
    void imagesLoaded();

protected:
    QVariant loadResource(int type, const QUrl &name);

    QQuickPixmap *loadPixmap(QQmlContext *context, const QUrl &name);

private slots:
    void requestFinished();

private:
    QHash<QUrl, QQuickPixmap *> m_resources;
    QUrl m_baseUrl;

    int outstanding;
    static QSet<QUrl> errors;
};

QT_END_NAMESPACE

#endif // QQUICKTEXT_P_P_H
