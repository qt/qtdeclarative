// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITCONTROLPROPERTIES_P_H
#define QQSTYLEKITCONTROLPROPERTIES_P_H

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
#include <QtGui/QColor>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/private/qquickimage_p.h>
#include <QtQuick/private/qquicktransition_p.h>
#include <QtQuick/private/qquicktext_p.h>

#include "qqstylekitglobal_p.h"
#include "qqstylekitpropertyresolver_p.h"
#include <QtLabsStyleKit/qtlabsstylekitexports.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QQStyleKitStyle;
class QQStyleKitControl;
class QQStyleKitControlState;
class QQStyleKitReader;
class QQStyleKitControlProperties;
class QQStyleKitDelegateProperties;

class QQStyleKitPropertyGroup: public QObject
{
    Q_OBJECT

public:
    enum class EmitFlag {
        AllProperties,
        Colors
    };
    Q_DECLARE_FLAGS(EmitFlags, EmitFlag)
    Q_FLAG(EmitFlag)

    QQStyleKitPropertyGroup(QQSK::PropertyGroup group, QObject *parent);

    PropertyPathId propertyPathId(QQSK::Property property, PropertyPathId::Flag flag) const;
    QString pathToString() const;

    template<typename T>
    inline T styleProperty(
        QQSK::Property property,
        QQSK::Property alternative = QQSK::Property::NoProperty) const
    {
        return qvariant_cast<T>(QQStyleKitPropertyResolver::readStyleProperty(this, property, alternative));
    }

    template<typename T>
    inline T styleProperty(QQSK::Property property, const T &defaultValue) const
    {
        const QVariant value = QQStyleKitPropertyResolver::readStyleProperty(this, property);
        return value.isValid() ? qvariant_cast<T>(value) : defaultValue;
    }

    template<typename T>
    inline bool setStyleProperty(QQSK::Property property, T value)
    {
        // This function will return true if the new value differes from the old one
        return QQStyleKitPropertyResolver::writeStyleProperty(this, property, QVariant::fromValue(value));
    }

    inline bool isDefined(QQSK::Property property) const
    {
        return QQStyleKitPropertyResolver::readStyleProperty(this, property).isValid();
    }

    template<typename SUBCLASS>
    inline void handleStylePropertyChanged(void (SUBCLASS::*changedSignal)());

    template <typename SUBCLASS, typename... CHANGED_SIGNALS>
    inline void handleStylePropertiesChanged(CHANGED_SIGNALS... changedSignals);

    template <typename T>
    T *lazyCreateGroup(T *const &ptr, QQSK::PropertyGroup group) const;

    inline bool isControlProperties() const
    {
        /* Only QQStyleKitControlProperties (as opposed to the nested delegates) have properties
         * with an ID at the bottom of the available space. The exception is the global flag, which
         * inherits the groupSpace from the control. */
        return m_groupSpace.start == 0 && m_pathFlags != QQSK::PropertyPathFlag::Global;
    }

    QQStyleKitControlProperties *controlProperties() const;
    inline QQSK::PropertyPathFlags pathFlags() const { return m_pathFlags; }
    void emitChangedForAllStylePropertiesRecursive(EmitFlags emitFlags);

protected:
    QQStyleKitPropertyGroupSpace m_groupSpace;
    QQSK::PropertyPathFlags m_pathFlags = QQSK::PropertyPathFlag::NoFlags;

private:
    bool shouldEmitLocally();
    bool shouldEmitGlobally();

    static QHash<PropertyPathId_t, QString> s_pathStrings;
};

// ************* QQStyleKitImageProperties ****************

class QQStyleKitImageProperties : public QQStyleKitPropertyGroup
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(QQuickImage::FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged FINAL)
    QML_UNCREATABLE("This component can only be instantiated by StyleKit")
    QML_NAMED_ELEMENT(ImageStyle)

public:
    QQStyleKitImageProperties(QQSK::PropertyGroup group, QQStyleKitControlProperties *parent = nullptr);

    template <typename... CHANGED_SIGNALS>
    void emitGlobally(QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const;

    QUrl source() const;
    void setSource(const QUrl &source);

    QColor color() const;
    void setColor(const QColor &color);

    QQuickImage::FillMode fillMode() const;
    void setFillMode(QQuickImage::FillMode fillMode);

signals:
    void sourceChanged();
    void colorChanged();
    void fillModeChanged();
};

// ************* QQStyleKitBorderProperties ****************

class QQStyleKitBorderProperties : public QQStyleKitPropertyGroup
{
    Q_OBJECT
    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    QML_UNCREATABLE("This component can only be instantiated by StyleKit")
    QML_NAMED_ELEMENT(BorderStyle)

public:
    QQStyleKitBorderProperties(QQSK::PropertyGroup group, QQStyleKitControlProperties *parent = nullptr);

    template <typename... CHANGED_SIGNALS>
    void emitGlobally(QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const;

    qreal width() const;
    void setWidth(qreal width);

    QColor color() const;
    void setColor(const QColor &color);

signals:
    void widthChanged();
    void colorChanged();
};

// ************* QQStyleKitShadowProperties ****************

class Q_LABSSTYLEKIT_EXPORT QQStyleKitShadowProperties : public QQStyleKitPropertyGroup
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged FINAL)
    Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY scaleChanged FINAL)
    Q_PROPERTY(qreal verticalOffset READ verticalOffset WRITE setVerticalOffset NOTIFY verticalOffsetChanged FINAL)
    Q_PROPERTY(qreal horizontalOffset READ horizontalOffset WRITE setHorizontalOffset NOTIFY horizontalOffsetChanged FINAL)
    Q_PROPERTY(qreal blur READ blur WRITE setBlur NOTIFY blurChanged FINAL)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    QML_UNCREATABLE("This component can only be instantiated by StyleKit")
    QML_NAMED_ELEMENT(ShadowStyle)

public:
    QQStyleKitShadowProperties(QQSK::PropertyGroup group, QQStyleKitControlProperties *parent = nullptr);

    template <typename... CHANGED_SIGNALS>
    void emitGlobally(QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const;

    QColor color() const;
    void setColor(QColor color);

    qreal opacity() const;
    void setOpacity(qreal opacity);

    qreal scale() const;
    void setScale(qreal scale);

    qreal verticalOffset() const;
    void setVerticalOffset(qreal verticalOffset);

    qreal horizontalOffset() const;
    void setHorizontalOffset(qreal horizontalOffset);

    qreal blur() const;
    void setBlur(qreal blur);

    bool visible() const;
    void setVisible(bool visible);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

signals:
    void colorChanged();
    void opacityChanged();
    void scaleChanged();
    void verticalOffsetChanged();
    void horizontalOffsetChanged();
    void blurChanged();
    void visibleChanged();
    void delegateChanged();
};

// ************* QQStyleKitDelegateProperties ****************

class Q_LABSSTYLEKIT_EXPORT QQStyleKitDelegateProperties : public QQStyleKitPropertyGroup
{
    Q_OBJECT
    Q_PROPERTY(qreal implicitWidth READ implicitWidth WRITE setImplicitWidth NOTIFY implicitWidthChanged FINAL)
    Q_PROPERTY(qreal implicitHeight READ implicitHeight WRITE setImplicitHeight NOTIFY implicitHeightChanged FINAL)
    Q_PROPERTY(qreal minimumWidth READ minimumWidth WRITE setMinimumWidth NOTIFY minimumWidthChanged FINAL)
    Q_PROPERTY(qreal margins READ margins WRITE setMargins NOTIFY marginsChanged FINAL)
    Q_PROPERTY(qreal leftMargin READ leftMargin WRITE setLeftMargin NOTIFY leftMarginChanged FINAL)
    Q_PROPERTY(qreal rightMargin READ rightMargin WRITE setRightMargin NOTIFY rightMarginChanged FINAL)
    Q_PROPERTY(qreal topMargin READ topMargin WRITE setTopMargin NOTIFY topMarginChanged FINAL)
    Q_PROPERTY(qreal bottomMargin READ bottomMargin WRITE setBottomMargin NOTIFY bottomMarginChanged FINAL)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged FINAL)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged FINAL)
    Q_PROPERTY(qreal topLeftRadius READ topLeftRadius WRITE setTopLeftRadius NOTIFY topLeftRadiusChanged FINAL)
    Q_PROPERTY(qreal topRightRadius READ topRightRadius WRITE setTopRightRadius NOTIFY topRightRadiusChanged FINAL)
    Q_PROPERTY(qreal bottomLeftRadius READ bottomLeftRadius WRITE setBottomLeftRadius NOTIFY bottomLeftRadiusChanged FINAL)
    Q_PROPERTY(qreal bottomRightRadius READ bottomRightRadius WRITE setBottomRightRadius NOTIFY bottomRightRadiusChanged FINAL)
    Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY scaleChanged FINAL)
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation NOTIFY rotationChanged FINAL)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(bool clip READ clip WRITE setClip NOTIFY clipChanged FINAL)
    Q_PROPERTY(QQuickGradient *gradient READ gradient WRITE setGradient NOTIFY gradientChanged FINAL)
    Q_PROPERTY(QQStyleKitImageProperties *image READ image NOTIFY imageChanged FINAL)
    Q_PROPERTY(QQStyleKitBorderProperties *border READ border NOTIFY borderChanged FINAL)
    Q_PROPERTY(QQStyleKitShadowProperties *shadow READ shadow NOTIFY shadowChanged FINAL)
    Q_PROPERTY(QObject *data READ data WRITE setData NOTIFY dataChanged FINAL)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    QML_UNCREATABLE("This component can only be instantiated by StyleKit")
    QML_NAMED_ELEMENT(DelegateStyle)

public:
    QQStyleKitDelegateProperties(QQSK::PropertyGroup group, QQStyleKitControlProperties *parent = nullptr);

    template <typename... CHANGED_SIGNALS>
    void emitGlobally(QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const;

    qreal radius() const;
    void setRadius(qreal radius);

    qreal topLeftRadius() const;
    void setTopLeftRadius(qreal radius);

    qreal topRightRadius() const;
    void setTopRightRadius(qreal radius);

    qreal bottomLeftRadius() const;
    void setBottomLeftRadius(qreal radius);

    qreal bottomRightRadius() const;
    void setBottomRightRadius(qreal radius);

    qreal scale() const;
    void setScale(qreal scale);

    qreal rotation() const;
    void setRotation(qreal rotation);

    qreal implicitWidth() const;
    void setImplicitWidth(qreal width);

    qreal implicitHeight() const;
    void setImplicitHeight(qreal height);

    qreal minimumWidth() const;
    void setMinimumWidth(qreal width);

    qreal margins() const;
    void setMargins(qreal margins);

    qreal leftMargin() const;
    void setLeftMargin(qreal margin);

    qreal rightMargin() const;
    void setRightMargin(qreal margin);

    qreal topMargin() const;
    void setTopMargin(qreal margin);

    qreal bottomMargin() const;
    void setBottomMargin(qreal margin);

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment alignment);

    qreal opacity() const;
    void setOpacity(qreal opacity);

    QColor color() const;
    void setColor(const QColor &color);

    bool visible() const;
    void setVisible(bool visible);

    bool clip() const;
    void setClip(bool clip);

    QQuickGradient *gradient() const;
    void setGradient(QQuickGradient *gradient);

    QObject *data() const;
    void setData(QObject *data);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    QQStyleKitImageProperties *image() const;
    QQStyleKitBorderProperties *border() const;
    QQStyleKitShadowProperties *shadow() const;

signals:
    void colorChanged();
    void radiusChanged();
    void topLeftRadiusChanged();
    void topRightRadiusChanged();
    void bottomLeftRadiusChanged();
    void bottomRightRadiusChanged();
    void scaleChanged();
    void rotationChanged();
    void visibleChanged();
    void clipChanged();
    void borderChanged();
    void shadowChanged();
    void imageChanged();
    void gradientChanged();
    void colorImageChanged();
    void implicitWidthChanged();
    void implicitHeightChanged();
    void minimumWidthChanged();
    void marginsChanged();
    void leftMarginChanged();
    void rightMarginChanged();
    void topMarginChanged();
    void bottomMarginChanged();
    void alignmentChanged();
    void opacityChanged();
    void dataChanged();
    void delegateChanged();

private:
    QPointer<QQuickGradient> m_gradient;
    QQStyleKitBorderProperties *m_border = nullptr;
    QQStyleKitShadowProperties *m_shadow = nullptr;
    QQStyleKitImageProperties *m_image = nullptr;
};

// ************* QQStyleKitHandleProperties ****************

class QQStyleKitHandleProperties : public QQStyleKitDelegateProperties
{
    Q_OBJECT
    Q_PROPERTY(QQStyleKitDelegateProperties *first READ first NOTIFY firstChanged FINAL)
    Q_PROPERTY(QQStyleKitDelegateProperties *second READ second NOTIFY secondChanged FINAL)
    QML_UNCREATABLE("This component can only be instantiated by StyleKit")
    QML_NAMED_ELEMENT(HandleStyle)

public:
    QQStyleKitHandleProperties(QQSK::PropertyGroup group, QQStyleKitControlProperties *parent = nullptr);
    QQStyleKitDelegateProperties *first() const;
    QQStyleKitDelegateProperties *second() const;

signals:
    void firstChanged();
    void secondChanged();

private:
    QQStyleKitDelegateProperties *m_first = nullptr;
    QQStyleKitDelegateProperties *m_second = nullptr;

    friend class QQStyleKitReader;
    friend class QQStyleKitControlProperties;
};

// ************* QQStyleKitIndicatorProperties ****************

class QQStyleKitIndicatorProperties : public QQStyleKitDelegateProperties
{
    Q_OBJECT
    Q_PROPERTY(QQStyleKitDelegateProperties *foreground READ foreground NOTIFY foregroundChanged FINAL)
    QML_UNCREATABLE("This component can only be instantiated by StyleKit")
    QML_NAMED_ELEMENT(SubIndicatorStyle)

public:
    QQStyleKitIndicatorProperties(QQSK::PropertyGroup group, QQStyleKitControlProperties *parent = nullptr);

    template <typename... CHANGED_SIGNALS>
    void emitGlobally(QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const;

    QQStyleKitDelegateProperties *foreground() const;

signals:
    void foregroundChanged();

private:
    QQStyleKitDelegateProperties *m_foreground = nullptr;

    friend class QQStyleKitReader;
    friend class QQStyleKitControlProperties;
};

// ************* QQStyleKitIndicatorWithSubTypes ****************

class QQStyleKitIndicatorWithSubTypes : public QQStyleKitDelegateProperties
{
    Q_OBJECT
    Q_PROPERTY(QQStyleKitDelegateProperties *foreground READ foreground NOTIFY foregroundChanged FINAL)
    Q_PROPERTY(QQStyleKitIndicatorProperties *up READ up NOTIFY upChanged FINAL)
    Q_PROPERTY(QQStyleKitIndicatorProperties *down READ down NOTIFY downChanged FINAL)
    QML_UNCREATABLE("This component can only be instantiated by StyleKit")
    QML_NAMED_ELEMENT(IndicatorStyle)

public:
    QQStyleKitIndicatorWithSubTypes(QQSK::PropertyGroup group, QQStyleKitControlProperties *parent = nullptr);

    template <typename... CHANGED_SIGNALS>
    void emitGlobally(QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const;

    QQStyleKitDelegateProperties *foreground() const;
    QQStyleKitIndicatorProperties *up() const;
    QQStyleKitIndicatorProperties *down() const;

signals:
    void foregroundChanged();
    void upChanged();
    void downChanged();

private:
    QQStyleKitDelegateProperties *m_foreground = nullptr;
    QQStyleKitIndicatorProperties *m_up = nullptr;
    QQStyleKitIndicatorProperties *m_down = nullptr;

    friend class QQStyleKitReader;
    friend class QQStyleKitControlProperties;
};

// ************* QQStyleKitTextProperties ****************

class QQStyleKitTextProperties : public QQStyleKitPropertyGroup
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged FINAL)
    Q_PROPERTY(bool bold READ bold WRITE setBold NOTIFY boldChanged FINAL)
    Q_PROPERTY(bool italic READ italic WRITE setItalic NOTIFY italicChanged FINAL)
    Q_PROPERTY(qreal pointSize READ pointSize WRITE setPointSize NOTIFY pointSizeChanged FINAL)
    Q_PROPERTY(qreal padding READ padding WRITE setPadding NOTIFY paddingChanged FINAL)
    Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding NOTIFY leftPaddingChanged FINAL)
    Q_PROPERTY(qreal rightPadding READ rightPadding WRITE setRightPadding NOTIFY rightPaddingChanged FINAL)
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding NOTIFY topPaddingChanged FINAL)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding NOTIFY bottomPaddingChanged FINAL)

    QML_UNCREATABLE("This component can only be instantiated by StyleKit")
    QML_NAMED_ELEMENT(TextStyle)

public:
    QQStyleKitTextProperties(QQSK::PropertyGroup group, QQStyleKitControlProperties *parent = nullptr);

    template <typename... CHANGED_SIGNALS>
    void emitGlobally(QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const;

    QColor color() const;
    void setColor(const QColor &color);

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment alignment);

    bool bold() const;
    void setBold(bool bold);

    bool italic() const;
    void setItalic(bool italic);

    qreal pointSize() const;
    void setPointSize(qreal pointSize);

    qreal padding() const;
    void setPadding(qreal padding);

    qreal leftPadding() const;
    void setLeftPadding(qreal leftPadding);

    qreal rightPadding() const;
    void setRightPadding(qreal rightPadding);

    qreal topPadding() const;
    void setTopPadding(qreal topPadding);

    qreal bottomPadding() const;
    void setBottomPadding(qreal bottomPadding);

signals:
    void colorChanged();
    void alignmentChanged();
    void boldChanged();
    void italicChanged();
    void pointSizeChanged();
    void paddingChanged();
    void leftPaddingChanged();
    void rightPaddingChanged();
    void topPaddingChanged();
    void bottomPaddingChanged();
};

/************* QQStyleKitControlProperties ****************
 * QQStyleKitControlProperties (and all other subclasses of QQStyleKitPropertyGroup),
 * is just an empty interface declaring the properties available for styling and reading.
 * It contains as little data as possible since it will be instantiated a lot. E.g a style
 * will instantiate them for every style.button, style.slider, etc defined, the same for a
 * theme, and also each instance of a StyleKitReader. Those are all subclasses of this
 * class. Each subclass will determine if the properties can be written to (as opposed to
 * only be read), and if so, offer a storage for storing those values. That storage is typically
 * a map that stores _only_ the properties that are written to, and nothing else.
 */
class QQStyleKitControlProperties : public QQStyleKitPropertyGroup
{
    Q_OBJECT
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged FINAL)
    Q_PROPERTY(qreal padding READ padding WRITE setPadding NOTIFY paddingChanged FINAL)
    Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding NOTIFY leftPaddingChanged FINAL)
    Q_PROPERTY(qreal rightPadding READ rightPadding WRITE setRightPadding NOTIFY rightPaddingChanged FINAL)
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding NOTIFY topPaddingChanged FINAL)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding NOTIFY bottomPaddingChanged FINAL)
    Q_PROPERTY(QQStyleKitDelegateProperties *background READ background NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(QQStyleKitHandleProperties *handle READ handle NOTIFY handleChanged FINAL)
    Q_PROPERTY(QQStyleKitIndicatorWithSubTypes *indicator READ indicator NOTIFY indicatorChanged FINAL)
    Q_PROPERTY(QQStyleKitTextProperties *text READ text NOTIFY textChanged FINAL)
    Q_PROPERTY(QQuickTransition *transition READ transition WRITE setTransition NOTIFY transitionChanged FINAL)

    QML_UNCREATABLE("This component can only be instantiated by StyleKit")
    QML_NAMED_ELEMENT(ControlStyleProperties)

public:
    QQStyleKitControlProperties(QQSK::PropertyGroup group, QObject *parent = nullptr);

    void emitChangedForAllStyleProperties(EmitFlags emitFlags);;
    template <typename... CHANGED_SIGNALS>
    void emitGlobally(QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const;
    void forEachUsedDelegate(
        std::function<void (QQStyleKitDelegateProperties *, QQSK::Delegate, const QString &)> f);

    QQStyleKitStyle *style() const;
    QQSK::Subclass subclass() const;
    QQStyleKitReader *asQQStyleKitReader() const;
    QQStyleKitControlState *asQQStyleKitState() const;

    qreal spacing() const;
    void setSpacing(qreal spacing);

    qreal padding() const;
    void setPadding(qreal padding);

    qreal leftPadding() const;
    void setLeftPadding(qreal leftPadding);

    qreal rightPadding() const;
    void setRightPadding(qreal rightPadding);

    qreal topPadding() const;
    void setTopPadding(qreal topPadding);

    qreal bottomPadding() const;
    void setBottomPadding(qreal bottomPadding);

    QQuickTransition* transition() const;
    void setTransition(QQuickTransition *transition);

    QQStyleKitTextProperties *text() const;

    QQStyleKitDelegateProperties *background() const;
    QQStyleKitHandleProperties *handle() const;
    QQStyleKitIndicatorWithSubTypes *indicator() const;

signals:
    void backgroundChanged();
    void handleChanged();
    void indicatorChanged();
    void spacingChanged();
    void paddingChanged();
    void leftPaddingChanged();
    void rightPaddingChanged();
    void topPaddingChanged();
    void bottomPaddingChanged();
    void transitionChanged();
    void textChanged();

private:
    Q_DISABLE_COPY(QQStyleKitControlProperties)

    QQStyleKitDelegateProperties *m_background = nullptr;
    QQStyleKitHandleProperties *m_handle = nullptr;
    QQStyleKitIndicatorWithSubTypes *m_indicator = nullptr;
    QQStyleKitTextProperties *m_text = nullptr;

    friend class QQStyleKitPropertyResolver;
    friend class QQStyleKitReader;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITCONTROLPROPERTIES_P_H
