/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKRENDERTARGET_P_H
#define QQUICKRENDERTARGET_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>
#include "qquickrendertarget.h"
#include <QAtomicInt>

QT_BEGIN_NAMESPACE

class QRhi;
class QQuickWindowRenderTarget;

class Q_QUICK_PRIVATE_EXPORT QQuickRenderTargetPrivate
{
public:
    static QQuickRenderTargetPrivate *get(QQuickRenderTarget *rt) { return rt->d; }
    static const QQuickRenderTargetPrivate *get(const QQuickRenderTarget *rt) { return rt->d; }
    QQuickRenderTargetPrivate();
    QQuickRenderTargetPrivate(const QQuickRenderTargetPrivate *other);
    bool resolve(QRhi *rhi, QQuickWindowRenderTarget *dst);

    enum class Type {
        Null,
        NativeTexture,
        NativeRenderbuffer,
        RhiRenderTarget,
        PaintDevice
    };

    QAtomicInt ref;
    Type type = Type::Null;
    QSize pixelSize;
    qreal devicePixelRatio = 1.0;
    int sampleCount = 1;
    struct NativeTexture {
        quint64 object;
        int layout;
    };
    union {
        NativeTexture nativeTexture;
        quint64 nativeRenderbufferObject;
        QRhiRenderTarget *rhiRt;
        QPaintDevice *paintDevice;
    } u;
};

QT_END_NAMESPACE

#endif // QQUICKRENDERTARGET_P_H
