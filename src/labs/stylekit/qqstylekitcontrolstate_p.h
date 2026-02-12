// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITCONTROLSTATE_P_H
#define QQSTYLEKITCONTROLSTATE_P_H

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

#include <QtQml/QtQml>

#include "qqstylekitcontrolproperties_p.h"

QT_BEGIN_NAMESPACE

class QQStyleKitControl;

class QQStyleKitControlState : public QQStyleKitControlProperties
{
    Q_OBJECT
    Q_PROPERTY(QQStyleKitControlState *pressed READ pressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(QQStyleKitControlState *hovered READ hovered NOTIFY hoveredChanged FINAL)
    Q_PROPERTY(QQStyleKitControlState *focused READ focused NOTIFY focusedChanged FINAL)
    Q_PROPERTY(QQStyleKitControlState *checked READ checked NOTIFY checkedChanged FINAL)
    Q_PROPERTY(QQStyleKitControlState *disabled READ disabled NOTIFY disabledChanged FINAL)
    Q_PROPERTY(QQStyleKitControlState *highlighted READ highlighted NOTIFY highlightedChanged FINAL)
    Q_PROPERTY(QQStyleKitControlState *vertical READ vertical NOTIFY verticalChanged FINAL)
    QML_NAMED_ELEMENT(ControlState)

public:
    QQStyleKitControlState(QObject *parent = nullptr);

    QQStyleKitControlState *pressed() const;
    QQStyleKitControlState *hovered() const;
    QQStyleKitControlState *focused() const;
    QQStyleKitControlState *checked() const;
    QQStyleKitControlState *disabled() const;
    QQStyleKitControlState *highlighted() const;
    QQStyleKitControlState *vertical() const;

    QQStyleKitControl *control() const;
    inline QQSK::State nestedState() const { return m_nestedState; }

signals:
    void pressedChanged();
    void hoveredChanged();
    void focusedChanged();
    void checkedChanged();
    void disabledChanged();
    void highlightedChanged();
    void verticalChanged();

private:
    Q_DISABLE_COPY(QQStyleKitControlState)
    QQStyleKitControlState *lazyCreateState(QQSK::StateFlag state) const;

private:
    QMap<QQSK::StateFlag, QQStyleKitControlState *> m_nestedStateObjects;
    QQSK::State m_nestedState = QQSK::StateFlag::Normal;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITCONTROLSTATE_P_H
