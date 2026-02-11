// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITDELEGATE_P_H
#define QQSTYLEKITDELEGATE_P_H

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

#include <QtQuick/private/qquickimplicitsizeitem_p.h>
#include <QtQml/qqmlparserstatus.h>

QT_BEGIN_NAMESPACE

class QQStyleKitDelegateProperties;

class QQStyleKitDelegate : public QQuickImplicitSizeItem
{
    Q_OBJECT
    Q_PROPERTY(QQStyleKitDelegateProperties *delegateStyle READ delegateStyle WRITE setDelegateStyle NOTIFY delegateStyleChanged REQUIRED FINAL)
    QML_NAMED_ELEMENT(StyledItemBase)

public:
    explicit QQStyleKitDelegate(QQuickItem *parent = nullptr);

    QQStyleKitDelegateProperties *delegateStyle() const;
    void setDelegateStyle(QQStyleKitDelegateProperties *delegateStyle);

signals:
    void delegateStyleChanged();

private:
    void updateImplicitSize();
    void maybeCreateColor();
    void maybeCreateGradient();
    void maybeCreateImage();

private:
    QPointer<QQStyleKitDelegateProperties> m_delegateProperties;
    QPointer<QQuickItem> m_colorOverlay;
    QPointer<QQuickItem> m_gradientOverlay;
    QPointer<QQuickItem> m_imageOverlay;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITDELEGATE_P_H
