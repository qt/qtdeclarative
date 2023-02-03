// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include <QtCore/private/qglobal_p.h>

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
    void requestFinished();

private:
    QHash<QUrl, QQuickPixmap *> m_resources;

    int outstanding;
    static QSet<QUrl> errors;
};

namespace QtPrivate {
class ProtectedLayoutAccessor: public QAbstractTextDocumentLayout
{
public:
    inline QTextCharFormat formatAccessor(int pos)
    {
        return format(pos);
    }
};
} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QQUICKDOCUMENT_P_H
