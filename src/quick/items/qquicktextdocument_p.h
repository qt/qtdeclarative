/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQUICKTEXTDOCUMENT_P_H
#define QQUICKTEXTDOCUMENT_P_H

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

#include <QtCore/qhash.h>
#include <QtCore/qvariant.h>
#include <QtGui/qimage.h>
#include <QtGui/qtextdocument.h>
#include <QtGui/qabstracttextdocumentlayout.h>
#include <QtGui/qtextlayout.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickPixmap;
class QQmlContext;

class Q_AUTOTEST_EXPORT QQuickTextDocumentWithImageResources : public QTextDocument, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
public:
    QQuickTextDocumentWithImageResources(QQuickItem *parent);
    virtual ~QQuickTextDocumentWithImageResources();

    void setText(const QString &);
#if QT_CONFIG(textmarkdownreader)
    void setMarkdownText(const QString &);
#endif
    int resourcesLoading() const { return outstanding; }

    QSizeF intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format) override;
    void drawObject(QPainter *p, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format) override;

    QImage image(const QTextImageFormat &format) const;

public Q_SLOTS:
    void clearResources();

Q_SIGNALS:
    void imagesLoaded();

protected:
    QVariant loadResource(int type, const QUrl &name) override;

    QQuickPixmap *loadPixmap(QQmlContext *context, const QUrl &name);

private Q_SLOTS:
    void reset();
    void requestFinished();

private:
    QHash<QUrl, QQuickPixmap *> m_resources;

    int outstanding;
    static QSet<QUrl> errors;
};

QT_END_NAMESPACE

#endif // QQUICKDOCUMENT_P_H
