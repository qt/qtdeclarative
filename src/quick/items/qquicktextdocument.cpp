/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
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
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicktextdocument.h"
#include "qquicktextdocument_p.h"

#include "qquicktextedit_p.h"
#include "qquicktextedit_p_p.h"
#include "qquicktext_p_p.h"

#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/private/qquickpixmapcache_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QQuickTextDocument
    \since 5.1
    \brief The QQuickTextDocument class provides access to the QTextDocument of QQuickTextEdit
    \inmodule QtQuick

    This class provides access to the QTextDocument of QQuickTextEdit elements.
    This is provided to allow usage of the \l{Rich Text Processing} functionalities of Qt.
    You are not allowed to modify the document, but it can be used to output content, for example with \l{QTextDocumentWriter}),
    or provide additional formatting, for example with \l{QSyntaxHighlighter}.

    The class has to be used from C++ directly, using the property of the \l TextEdit.

    Warning: The QTextDocument provided is used internally by \l {Qt Quick} elements to provide text manipulation primitives.
    You are not allowed to perform any modification of the internal state of the QTextDocument. If you do, the element
    in question may stop functioning or crash.
*/

class QQuickTextDocumentPrivate : public QObjectPrivate
{
public:
    QPointer<QTextDocument> document;
};

/*!
   Constructs a QQuickTextDocument object with
   \a parent as the parent object.
*/
QQuickTextDocument::QQuickTextDocument(QQuickItem *parent)
    : QObject(*(new QQuickTextDocumentPrivate), parent)
{
    Q_D(QQuickTextDocument);
    Q_ASSERT(parent);
    Q_ASSERT(qobject_cast<QQuickTextEdit*>(parent));
    d->document = QPointer<QTextDocument>(qobject_cast<QQuickTextEdit*>(parent)->d_func()->document);
}

/*!
   Returns a pointer to the QTextDocument object.
*/
QTextDocument* QQuickTextDocument::textDocument() const
{
    Q_D(const QQuickTextDocument);
    return d->document.data();
}

QQuickTextDocumentWithImageResources::QQuickTextDocumentWithImageResources(QQuickItem *parent)
: QTextDocument(parent), outstanding(0)
{
    setUndoRedoEnabled(false);
    documentLayout()->registerHandler(QTextFormat::ImageObject, this);
    connect(this, SIGNAL(baseUrlChanged(QUrl)), this, SLOT(reset()));
}

QQuickTextDocumentWithImageResources::~QQuickTextDocumentWithImageResources()
{
    if (!m_resources.isEmpty())
        qDeleteAll(m_resources);
}

QVariant QQuickTextDocumentWithImageResources::loadResource(int type, const QUrl &name)
{
    QVariant resource = QTextDocument::loadResource(type, name);
    if (resource.isNull() && type == QTextDocument::ImageResource) {
        QQmlContext *context = qmlContext(parent());
        QUrl url = baseUrl().resolved(name);
        QQuickPixmap *p = loadPixmap(context, url);
        resource = p->image();
    }

    return resource;
}

void QQuickTextDocumentWithImageResources::requestFinished()
{
    outstanding--;
    if (outstanding == 0) {
        markContentsDirty(0, characterCount());
        emit imagesLoaded();
    }
}

QSizeF QQuickTextDocumentWithImageResources::intrinsicSize(
        QTextDocument *, int, const QTextFormat &format)
{
    if (format.isImageFormat()) {
        QTextImageFormat imageFormat = format.toImageFormat();

        const int width = qRound(imageFormat.width());
        const bool hasWidth = imageFormat.hasProperty(QTextFormat::ImageWidth) && width > 0;
        const int height = qRound(imageFormat.height());
        const bool hasHeight = imageFormat.hasProperty(QTextFormat::ImageHeight) && height > 0;

        QSizeF size(width, height);
        if (!hasWidth || !hasHeight) {
            QVariant res = resource(QTextDocument::ImageResource, QUrl(imageFormat.name()));
            QImage image = res.value<QImage>();
            if (image.isNull()) {
                if (!hasWidth)
                    size.setWidth(16);
                if (!hasHeight)
                    size.setHeight(16);
                return size;
            }
            QSize imgSize = image.size();

            if (!hasWidth) {
                if (!hasHeight)
                    size.setWidth(imgSize.width());
                else
                    size.setWidth(qRound(height * (imgSize.width() / (qreal) imgSize.height())));
            }
            if (!hasHeight) {
                if (!hasWidth)
                    size.setHeight(imgSize.height());
                else
                    size.setHeight(qRound(width * (imgSize.height() / (qreal) imgSize.width())));
            }
        }
        return size;
    }
    return QSizeF();
}

void QQuickTextDocumentWithImageResources::drawObject(
        QPainter *, const QRectF &, QTextDocument *, int, const QTextFormat &)
{
}

QImage QQuickTextDocumentWithImageResources::image(const QTextImageFormat &format)
{
    QVariant res = resource(QTextDocument::ImageResource, QUrl(format.name()));
    return res.value<QImage>();
}

void QQuickTextDocumentWithImageResources::reset()
{
    clearResources();
    markContentsDirty(0, characterCount());
}

QQuickPixmap *QQuickTextDocumentWithImageResources::loadPixmap(
        QQmlContext *context, const QUrl &url)
{

    QHash<QUrl, QQuickPixmap *>::Iterator iter = m_resources.find(url);

    if (iter == m_resources.end()) {
        QQuickPixmap *p = new QQuickPixmap(context->engine(), url);
        iter = m_resources.insert(url, p);

        if (p->isLoading()) {
            p->connectFinished(this, SLOT(requestFinished()));
            outstanding++;
        }
    }

    QQuickPixmap *p = *iter;
    if (p->isError()) {
        if (!errors.contains(url)) {
            errors.insert(url);
            qmlInfo(parent()) << p->error();
        }
    }
    return p;
}

void QQuickTextDocumentWithImageResources::clearResources()
{
    foreach (QQuickPixmap *pixmap, m_resources)
        pixmap->clear(this);
    qDeleteAll(m_resources);
    m_resources.clear();
    outstanding = 0;
}

void QQuickTextDocumentWithImageResources::setText(const QString &text)
{
    clearResources();

#ifndef QT_NO_TEXTHTMLPARSER
    setHtml(text);
#else
    setPlainText(text);
#endif
}

QSet<QUrl> QQuickTextDocumentWithImageResources::errors;

QT_END_NAMESPACE
