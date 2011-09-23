/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "private/qvariantanimation2_p.h"
#include "private/qvariantanimation2_p_p.h"

#include <QtCore/qrect.h>
#include <QtCore/qline.h>
#include <QtCore/qmutex.h>
#include <private/qmutexpool_p.h>



QT_BEGIN_NAMESPACE

/*!
    \class QVariantAnimation2
    \ingroup animation
    \brief The QVariantAnimation2 class provides an abstract base class for animations.
    \since 4.6

    This class is part of \l{The Animation Framework}. It serves as a
    base class for property and item animations, with functions for
    shared functionality.

    QVariantAnimation2 cannot be used directly as it is an abstract
    class; it has a pure virtual method called updateCurrentValue().
    The class performs interpolation over
    \l{QVariant}s, but leaves using the interpolated values to its
    subclasses. Currently, Qt provides QPropertyAnimation2, which
    animates Qt \l{Qt's Property System}{properties}. See the
    QPropertyAnimation2 class description if you wish to animate such
    properties.

    You can then set start and end values for the property by calling
    setStartValue() and setEndValue(), and finally call start() to
    start the animation. QVariantAnimation2 will interpolate the
    property of the target object and emit valueChanged(). To react to
    a change in the current value you have to reimplement the
    updateCurrentValue() virtual function.

    It is also possible to set values at specified steps situated
    between the start and end value. The interpolation will then
    touch these points at the specified steps. Note that the start and
    end values are defined as the key values at 0.0 and 1.0.

    There are two ways to affect how QVariantAnimation2 interpolates
    the values. You can set an easing curve by calling
    setEasingCurve(), and configure the duration by calling
    setDuration(). You can change how the QVariants are interpolated
    by creating a subclass of QVariantAnimation2, and reimplementing
    the virtual interpolated() function.

    Subclassing QVariantAnimation2 can be an alternative if you have
    \l{QVariant}s that you do not wish to declare as Qt properties.
    Note, however, that you in most cases will be better off declaring
    your QVariant as a property.

    Not all QVariant types are supported. Below is a list of currently
    supported QVariant types:

    \list
        \o \l{QMetaType::}{Int}
        \o \l{QMetaType::}{Double}
        \o \l{QMetaType::}{Float}
        \o \l{QMetaType::}{QLine}
        \o \l{QMetaType::}{QLineF}
        \o \l{QMetaType::}{QPoint}
        \o \l{QMetaType::}{QPointF}
        \o \l{QMetaType::}{QSize}
        \o \l{QMetaType::}{QSizeF}
        \o \l{QMetaType::}{QRect}
        \o \l{QMetaType::}{QRectF}
        \o \l{QMetaType::}{QColor}
    \endlist

    If you need to interpolate other variant types, including custom
    types, you have to implement interpolation for these yourself.
    To do this, you can register an interpolator function for a given
    type. This function takes 3 parameters: the start value, the end value
    and the current progress.

    Example:
    \code
        QVariant myColorInterpolator(const QColor &start, const QColor &end, qreal progress)
        {
            ...
            return QColor(...);
        }
        ...
        qRegisterAnimationInterpolator<QColor>(myColorInterpolator);
    \endcode

    Another option is to reimplement interpolated(), which returns
    interpolation values for the value being interpolated.

    \omit We need some snippets around here. \endomit

    \sa QPropertyAnimation2, QAbstractAnimation2, {The Animation Framework}
*/

/*!
    \fn void QVariantAnimation2::valueChanged(const QVariant &value)

    QVariantAnimation2 emits this signal whenever the current \a value changes.

    \sa currentValue, startValue, endValue
*/

/*!
    \fn void QVariantAnimation2::updateCurrentValue(const QVariant &value) = 0;

    This pure virtual function is called every time the animation's current
    value changes. The \a value argument is the new current value.

    \sa currentValue
*/

static bool animationValueLessThan(const QVariantAnimation2::KeyValue &p1, const QVariantAnimation2::KeyValue &p2)
{
    return p1.first < p2.first;
}

QVariantAnimation2Private::QVariantAnimation2Private() : currentValue(0), duration(250)
{
    keyValues << qMakePair(qreal(0), qreal(0)) << qMakePair(qreal(1), qreal(1));
    currentInterval.start = keyValues.at(0);
    currentInterval.end = keyValues.at(1);
    /*FIXME: anything else needed here?*/
}

/*!
    \internal
    The goal of this function is to update the currentInterval member. As a consequence, we also
    need to update the currentValue.
    Set \a force to true to always recalculate the interval.
*/
void QVariantAnimation2Private::recalculateCurrentInterval(bool force/*=false*/)
{
    // can't interpolate if we don't have at least 2 values
    if (keyValues.count() < 2)
        return;

    const qreal progress = easing.valueForProgress(((duration == 0) ? qreal(1) : qreal(currentTime) / qreal(duration)));

    //0 and 1 are still the boundaries
    if (force || (currentInterval.start.first > 0 && progress < currentInterval.start.first)
        || (currentInterval.end.first < 1 && progress > currentInterval.end.first)) {
        //let's update currentInterval
        QVariantAnimation2::KeyValues::const_iterator it = qLowerBound(keyValues.constBegin(),
                                                                      keyValues.constEnd(),
                                                                      qMakePair(progress, qreal(-1)),
                                                                      animationValueLessThan);
        if (it == keyValues.constBegin()) {
            //the item pointed to by it is the start element in the range    
            if (it->first == 0 && keyValues.count() > 1) {
                currentInterval.start = *it;
                currentInterval.end = *(it+1);
            }
        } else if (it == keyValues.constEnd()) {
            --it; //position the iterator on the last item
            if (it->first == 1 && keyValues.count() > 1) {
                //we have an end value (item with progress = 1)
                currentInterval.start = *(it-1);
                currentInterval.end = *it;
            }
        } else {
            currentInterval.start = *(it-1);
            currentInterval.end = *it;
        }
    }
    setCurrentValueForProgress(progress);
}

void QVariantAnimation2Private::setCurrentValueForProgress(const qreal progress)
{
    Q_Q(QVariantAnimation2);

    const qreal startProgress = currentInterval.start.first;
    const qreal endProgress = currentInterval.end.first;
    const qreal localProgress = (progress - startProgress) / (endProgress - startProgress);

    qreal ret = q->interpolated(currentInterval.start.second,
                                   currentInterval.end.second,
                                   localProgress);
    qSwap(currentValue, ret);
    q->updateCurrentValue(currentValue);
    static QBasicAtomicInt changedSignalIndex = Q_BASIC_ATOMIC_INITIALIZER(0);
    if (!changedSignalIndex) {
        //we keep the mask so that we emit valueChanged only when needed (for performance reasons)
        changedSignalIndex.testAndSetRelaxed(0, signalIndex("valueChanged(QVariant)"));
    }
    if (isSignalConnected(changedSignalIndex) && currentValue != ret) {
        //the value has changed
        emit q->valueChanged(currentValue);
    }
}

qreal QVariantAnimation2Private::valueAt(qreal step) const
{
    QVariantAnimation2::KeyValues::const_iterator result =
        qBinaryFind(keyValues.begin(), keyValues.end(), qMakePair(step, qreal(-1)/*FIXME*/), animationValueLessThan);
    if (result != keyValues.constEnd())
        return result->second;

    return -1; /*FIXME*/
}

void QVariantAnimation2Private::setValueAt(qreal step, const qreal &value)
{
    if (step < qreal(0.0) || step > qreal(1.0)) {
        qWarning("QVariantAnimation2::setValueAt: invalid step = %f", step);
        return;
    }

    QVariantAnimation2::KeyValue pair(step, value);

    QVariantAnimation2::KeyValues::iterator result = qLowerBound(keyValues.begin(), keyValues.end(), pair, animationValueLessThan);
    if (result == keyValues.end() || result->first != step) {
        keyValues.insert(result, pair);
    } else {
        result->second = value; // replaces the previous value
    }

    recalculateCurrentInterval(/*force=*/true);
}

/*!
    Construct a QVariantAnimation2 object. \a parent is passed to QAbstractAnimation2's
    constructor.
*/
QVariantAnimation2::QVariantAnimation2(QObject *parent) : QAbstractAnimation2(*new QVariantAnimation2Private, parent)
{
}

/*!
    \internal
*/
QVariantAnimation2::QVariantAnimation2(QVariantAnimation2Private &dd, QObject *parent) : QAbstractAnimation2(dd, parent)
{
}

/*!
    Destroys the animation.
*/
QVariantAnimation2::~QVariantAnimation2()
{
}

/*!
    \property QVariantAnimation2::easingCurve
    \brief the easing curve of the animation

    This property defines the easing curve of the animation. By
    default, a linear easing curve is used, resulting in linear
    interpolation. Other curves are provided, for instance,
    QEasingCurve::InCirc, which provides a circular entry curve.
    Another example is QEasingCurve::InOutElastic, which provides an
    elastic effect on the values of the interpolated variant.

    QVariantAnimation2 will use the QEasingCurve::valueForProgress() to
    transform the "normalized progress" (currentTime / totalDuration)
    of the animation into the effective progress actually
    used by the animation. It is this effective progress that will be
    the progress when interpolated() is called. Also, the steps in the
    keyValues are referring to this effective progress.

    The easing curve is used with the interpolator, the interpolated()
    virtual function, the animation's duration, and iterationCount, to
    control how the current value changes as the animation progresses.
*/
QEasingCurve QVariantAnimation2::easingCurve() const
{
    Q_D(const QVariantAnimation2);
    return d->easing;
}

void QVariantAnimation2::setEasingCurve(const QEasingCurve &easing)
{
    Q_D(QVariantAnimation2);
    d->easing = easing;
    d->recalculateCurrentInterval();
}


/*!
    \property QVariantAnimation2::duration
    \brief the duration of the animation

    This property describes the duration in milliseconds of the
    animation. The default duration is 250 milliseconds.

    \sa QAbstractAnimation2::duration()
 */
int QVariantAnimation2::duration() const
{
    Q_D(const QVariantAnimation2);
    return d->duration;
}

void QVariantAnimation2::setDuration(int msecs)
{
    Q_D(QVariantAnimation2);
    if (msecs < 0) {
        qWarning("QVariantAnimation2::setDuration: cannot set a negative duration");
        return;
    }
    if (d->duration == msecs)
        return;
    d->duration = msecs;
    d->recalculateCurrentInterval();
}

/*!
    Returns the key frame value for the given \a step. The given \a step
    must be in the range 0 to 1. If there is no KeyValue for \a step,
    it returns an invalid QVariant.

    \sa keyValues(), setKeyValueAt()
*/
qreal QVariantAnimation2::keyValueAt(qreal step) const
{
    return d_func()->valueAt(step);
}

/*!
    \typedef QVariantAnimation2::KeyValue

    This is a typedef for QPair<qreal, qreal>.
*/
/*!
    \typedef QVariantAnimation2::KeyValues

    This is a typedef for QVector<KeyValue>
*/

/*!
    Creates a key frame at the given \a step with the given \a value.
    The given \a step must be in the range 0 to 1.

    \sa setKeyValues(), keyValueAt()
*/
void QVariantAnimation2::setKeyValueAt(qreal step, const qreal &value)
{
    d_func()->setValueAt(step, value);
}

/*!
    Returns the key frames of this animation.

    \sa keyValueAt(), setKeyValues()
*/
QVariantAnimation2::KeyValues QVariantAnimation2::keyValues() const
{
    return d_func()->keyValues;
}

/*!
    Replaces the current set of key frames with the given \a keyValues.
    the step of the key frames must be in the range 0 to 1.

    \sa keyValues(), keyValueAt()
*/
void QVariantAnimation2::setKeyValues(const KeyValues &keyValues)
{
    Q_D(QVariantAnimation2);
    d->keyValues = keyValues;
    qSort(d->keyValues.begin(), d->keyValues.end(), animationValueLessThan);
    d->recalculateCurrentInterval(/*force=*/true);
}

/*!
    \property QVariantAnimation2::currentValue
    \brief the current value of the animation.

    This property describes the current value; an interpolated value
    between the \l{startValue}{start value} and the \l{endValue}{end
    value}, using the current time for progress. The value itself is
    obtained from interpolated(), which is called repeatedly as the
    animation is running.

    QVariantAnimation2 calls the virtual updateCurrentValue() function
    when the current value changes. This is particularly useful for
    subclasses that need to track updates. For example,
    QPropertyAnimation2 uses this function to animate Qt \l{Qt's
    Property System}{properties}.

    \sa startValue, endValue
*/
qreal QVariantAnimation2::currentValue() const
{
    Q_D(const QVariantAnimation2);
/*FIXME*/
//    if (!d->currentValue.isValid())
//        const_cast<QVariantAnimation2Private*>(d)->recalculateCurrentInterval();
    return d->currentValue;
}

/*!
    \reimp
 */
bool QVariantAnimation2::event(QEvent *event)
{
    return QAbstractAnimation2::event(event);
}

/*!
    \reimp
*/
void QVariantAnimation2::updateState(QAbstractAnimation2::State newState,
                                    QAbstractAnimation2::State oldState)
{
    Q_UNUSED(oldState);
    Q_UNUSED(newState);
}

/*!

    This virtual function returns the linear interpolation between
    variants \a from and \a to, at \a progress, usually a value
    between 0 and 1. You can reimplement this function in a subclass
    of QVariantAnimation2 to provide your own interpolation algorithm.

    Note that in order for the interpolation to work with a
    QEasingCurve that return a value smaller than 0 or larger than 1
    (such as QEasingCurve::InBack) you should make sure that it can
    extrapolate. If the semantic of the datatype does not allow
    extrapolation this function should handle that gracefully.

    You should call the QVariantAnimation2 implementation of this
    function if you want your class to handle the types already
    supported by Qt (see class QVariantAnimation2 description for a
    list of supported types).

    \sa QEasingCurve
 */
qreal QVariantAnimation2::interpolated(const qreal &from, const qreal &to, qreal progress) const
{
    return (from + (to - from) * progress);
}

/*!
    \reimp
 */
void QVariantAnimation2::updateCurrentTime(int)
{
    d_func()->recalculateCurrentInterval();
}

QT_END_NAMESPACE

#include "moc_qvariantanimation2_p.cpp"


