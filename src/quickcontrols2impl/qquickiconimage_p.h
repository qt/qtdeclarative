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

#ifndef QQUICKICONIMAGE_P_H
#define QQUICKICONIMAGE_P_H

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

#include <QtQuick/private/qquickimage_p.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickIconImagePrivate;

class Q_QUICKCONTROLS2_PRIVATE_EXPORT QQuickIconImage : public QQuickImage
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    QML_NAMED_ELEMENT(IconImage)
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickIconImage(QQuickItem *parent = nullptr);

    QString name() const;
    void setName(const QString &name);

    QColor color() const;
    void setColor(const QColor &color);

    void setSource(const QUrl &url) override;

Q_SIGNALS:
    void nameChanged();
    void colorChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;
    void pixmapChange() override;

private:
    Q_DISABLE_COPY(QQuickIconImage)
    Q_DECLARE_PRIVATE(QQuickIconImage)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickIconImage)

#endif // QQUICKICONIMAGE_P_H
