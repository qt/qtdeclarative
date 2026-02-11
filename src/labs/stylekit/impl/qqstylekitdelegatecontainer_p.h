// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITDELEGATECONTAINER_P_H
#define QQSTYLEKITDELEGATECONTAINER_P_H

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

#include <QtQuick/qquickitem.h>
#include <QtQml/qqmlparserstatus.h>

QT_BEGIN_NAMESPACE

class QQStyleKitDelegateProperties;

class QQStyleKitDelegateContainer : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQStyleKitDelegateProperties *delegateStyle READ delegateStyle WRITE setDelegateStyle NOTIFY delegateStyleChanged FINAL)
    Q_PROPERTY(QObject *quickControl READ quickControl WRITE setQuickControl NOTIFY quickControlChanged REQUIRED FINAL)
    Q_PROPERTY(QQuickItem *delegateInstance READ delegateInstance NOTIFY delegateInstanceChanged FINAL)
    Q_PROPERTY(bool usingDefaultDelegate READ usingDefaultDelegate NOTIFY usingDefaultDelegateChanged FINAL)
    QML_NAMED_ELEMENT(DelegateContainer)

public:
    explicit QQStyleKitDelegateContainer(QQuickItem *parent = nullptr);
    ~QQStyleKitDelegateContainer() override;

    QQStyleKitDelegateProperties *delegateStyle() const;
    void setDelegateStyle(QQStyleKitDelegateProperties *delegateProperties);

    QObject *quickControl() const;
    void setQuickControl(QObject *control);

    QQuickItem *delegateInstance() const;
    bool usingDefaultDelegate() const;

signals:
    void delegateStyleChanged();
    void quickControlChanged();
    void delegateInstanceChanged();
    void usingDefaultDelegateChanged();

protected:
    void componentComplete() override;

private:
    void updateImplicitSize();
    void maybeCreateDelegate();
    void maybeCreateShadow();

private:
    QPointer<QQStyleKitDelegateProperties> m_delegateProperties;
    QPointer<QObject> m_control;

    QPointer<QQuickItem> m_delegateInstance;
    QPointer<QQuickItem> m_shadowInstance;

    QPointer<QQmlComponent> m_delegateComponent;
    QPointer<QQmlComponent> m_shadowComponent;

    static QQmlComponent *s_defaultDelegateComponent;
    static QQmlComponent *s_defaultShadowComponent;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITDELEGATECONTAINER_P_H
