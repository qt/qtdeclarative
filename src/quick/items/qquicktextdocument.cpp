// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktextdocument.h"
#include "qquicktextdocument_p.h"

#include "qquicktextedit_p.h"
#include "qquicktextedit_p_p.h"
#include "qquicktext_p_p.h"

#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/private/qquickpixmap_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

/*!
    \class QQuickTextDocument
    \since 5.1
    \brief The QQuickTextDocument class provides access to the QTextDocument of QQuickTextEdit.
    \inmodule QtQuick

    This class provides access to the QTextDocument of QQuickTextEdit elements.
    This is provided to allow usage of the \l{Rich Text Processing} functionalities of Qt,
    including document modifications. It can also be used to output content,
    for example with \l{QTextDocumentWriter}), or provide additional formatting,
    for example with \l{QSyntaxHighlighter}.

    This class cannot be instantiated in QML, but is available from \l TextEdit::textDocument.
*/

/*!
   Constructs a QQuickTextDocument object with
   \a parent as the parent object.
*/
QQuickTextDocument::QQuickTextDocument(QQuickItem *parent)
    : QObject(*(new QQuickTextDocumentPrivate), parent)
{
    Q_D(QQuickTextDocument);
    Q_ASSERT(parent);
    d->editor = qobject_cast<QQuickTextEdit *>(parent);
    Q_ASSERT(d->editor);
}

QTextDocument *QQuickTextDocumentPrivate::document() const
{
    return editor->document();
}

void QQuickTextDocumentPrivate::setDocument(QTextDocument *doc)
{
    Q_Q(QQuickTextDocument);
    if (doc == editor->document())
        return;

    editor->setDocument(doc);
    emit q->textDocumentChanged();
}

/*!
   Returns a pointer to the QTextDocument object.
*/
QTextDocument *QQuickTextDocument::textDocument() const
{
    Q_D(const QQuickTextDocument);
    return d->document();
}

/*!
    \brief Sets the given \a document.
    \since 6.7

    The caller retains ownership of the document.
*/
void QQuickTextDocument::setTextDocument(QTextDocument *document)
{
    d_func()->setDocument(document);
}

QQuickTextImageHandler::QQuickTextImageHandler(QObject *parent)
    : QObject(parent)
{
}

QSizeF QQuickTextImageHandler::intrinsicSize(
        QTextDocument *doc, int, const QTextFormat &format)
{
    if (format.isImageFormat()) {
        QTextImageFormat imageFormat = format.toImageFormat();
        const int width = qRound(imageFormat.width());
        const bool hasWidth = imageFormat.hasProperty(QTextFormat::ImageWidth) && width > 0;
        const int height = qRound(imageFormat.height());
        const bool hasHeight = imageFormat.hasProperty(QTextFormat::ImageHeight) && height > 0;
        QSizeF size(width, height);
        if (!hasWidth || !hasHeight) {
            QVariant res = doc->resource(QTextDocument::ImageResource, QUrl(imageFormat.name()));
            QImage image = res.value<QImage>();
            if (image.isNull()) {
                // autotests expect us to reserve a 16x16 space for a "broken image" icon,
                // even though we don't actually display one
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

QT_END_NAMESPACE

#include "moc_qquicktextdocument.cpp"
#include "moc_qquicktextdocument_p.cpp"
