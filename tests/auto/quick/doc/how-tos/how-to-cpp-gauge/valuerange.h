// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef VALUERANGE_H
#define VALUERANGE_H

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class ValueRange : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal minimumValue READ minimumValue WRITE setMinimumValue NOTIFY minimumValueChanged FINAL)
    Q_PROPERTY(qreal maximumValue READ maximumValue WRITE setMaximumValue NOTIFY maximumValueChanged FINAL)
    Q_PROPERTY(qreal stepSize READ stepSize WRITE setStepSize NOTIFY stepSizeChanged FINAL)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged FINAL)
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL)
    Q_PROPERTY(bool inverted READ inverted WRITE setInverted NOTIFY invertedChanged FINAL)

    QML_ELEMENT
    Q_INTERFACES(QQmlParserStatus)

public:
    ValueRange(QObject *parent = 0);
    virtual ~ValueRange() = default;

    qreal value() const;
    void setValue(qreal value);

    qreal minimumValue() const;
    void setMinimumValue(qreal min);

    qreal maximumValue() const;
    void setMaximumValue(qreal max);

    void setStepSize(qreal stepSize);
    qreal stepSize() const;

    qreal position() const;
    qreal visualPosition() const;

    void setInverted(bool inverted);
    bool inverted() const;

    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void valueChanged();
    void positionChanged();
    void visualPositionChanged();
    void stepSizeChanged();
    void invertedChanged();
    void minimumValueChanged();
    void maximumValueChanged();
    // Emitted when things that affect the range (e.g. min, max, etc.) are changed.
    void constraintsChanged();

private:
    Q_DISABLE_COPY(ValueRange)

    void setPosition(qreal pos);
    void updatePosition();

    qreal mMinimumValue = 0;
    qreal mMaximumValue = 100;
    qreal mStepSize = 10;
    qreal mValue = 0;
    qreal mPosition = 0;
    qreal mPosAtMin = 0;
    qreal mPosAtMax = 0;
    // Set in constructor due to warning about C++20 flag being required.
    uint mInverted : 1;
    bool mIsComplete = false;
};

QT_END_NAMESPACE

#endif // VALUERANGE_H
