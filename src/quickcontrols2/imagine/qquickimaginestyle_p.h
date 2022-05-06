/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQUICKIMAGINESTYLE_P_H
#define QQUICKIMAGINESTYLE_P_H

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

#include <QtCore/qvariant.h>
#include <QtQml/qqml.h>
#include <QtQuickControls2Impl/private/qquickattachedobject_p.h>

QT_BEGIN_NAMESPACE

class QQuickImagineStyle : public QQuickAttachedObject
{
    Q_OBJECT
    Q_PROPERTY(QString path READ path WRITE setPath RESET resetPath NOTIFY pathChanged FINAL)
    Q_PROPERTY(QUrl url READ url NOTIFY pathChanged FINAL)
    QML_NAMED_ELEMENT(Imagine)
    QML_ATTACHED(QQuickImagineStyle)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickImagineStyle(QObject *parent = nullptr);

    static QQuickImagineStyle *qmlAttachedProperties(QObject *object);

    QString path() const;
    void setPath(const QString &path);
    void inheritPath(const QString &path);
    void propagatePath();
    void resetPath();

    QUrl url() const;

Q_SIGNALS:
    void pathChanged();

protected:
    void attachedParentChange(QQuickAttachedObject *newParent, QQuickAttachedObject *oldParent) override;

private:
    void init();

    bool m_explicitPath = false;
    QString m_path;
};

QT_END_NAMESPACE

QML_DECLARE_TYPEINFO(QQuickImagineStyle, QML_HAS_ATTACHED_PROPERTIES)

#endif // QQUICKIMAGINESTYLE_P_H
