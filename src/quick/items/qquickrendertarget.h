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

#ifndef QQUICKRENDERTARGET_H
#define QQUICKRENDERTARGET_H

#include <QtQuick/qtquickglobal.h>
#include <QtCore/qsize.h>

#if QT_CONFIG(vulkan)
#include <QtGui/qvulkaninstance.h>
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS) || defined(Q_CLANG_QDOC)
Q_FORWARD_DECLARE_OBJC_CLASS(MTLTexture);
#endif

QT_BEGIN_NAMESPACE

class QQuickRenderTargetPrivate;
class QRhiRenderTarget;
class QPaintDevice;

class Q_QUICK_EXPORT QQuickRenderTarget
{
public:
    QQuickRenderTarget();
    ~QQuickRenderTarget();
    QQuickRenderTarget(const QQuickRenderTarget &other);
    QQuickRenderTarget &operator=(const QQuickRenderTarget &other);

    bool isNull() const;

    qreal devicePixelRatio() const;
    void setDevicePixelRatio(qreal ratio);

#if QT_CONFIG(opengl) || defined(Q_CLANG_QDOC)
    static QQuickRenderTarget fromOpenGLTexture(uint textureId, const QSize &pixelSize, int sampleCount = 1);
    static QQuickRenderTarget fromOpenGLRenderBuffer(uint renderbufferId, const QSize &pixelSize, int sampleCount = 1);
#endif

#if defined(Q_OS_WIN) || defined(Q_CLANG_QDOC)
    static QQuickRenderTarget fromD3D11Texture(void *texture, const QSize &pixelSize, int sampleCount = 1);
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS) || defined(Q_CLANG_QDOC)
    static QQuickRenderTarget fromMetalTexture(MTLTexture *texture, const QSize &pixelSize, int sampleCount = 1);
#endif

#if QT_CONFIG(vulkan) || defined(Q_CLANG_QDOC)
    static QQuickRenderTarget fromVulkanImage(VkImage image, VkImageLayout layout, const QSize &pixelSize, int sampleCount = 1);
#endif

    static QQuickRenderTarget fromRhiRenderTarget(QRhiRenderTarget *renderTarget);

    static QQuickRenderTarget fromPaintDevice(QPaintDevice *device);

private:
    void detach();
    bool isEqual(const QQuickRenderTarget &other) const noexcept;
    QQuickRenderTargetPrivate *d;
    friend class QQuickRenderTargetPrivate;

    friend bool operator==(const QQuickRenderTarget &lhs, const QQuickRenderTarget &rhs) noexcept
    { return lhs.isEqual(rhs); }
    friend bool operator!=(const QQuickRenderTarget &lhs, const QQuickRenderTarget &rhs) noexcept
    { return !lhs.isEqual(rhs); }
};

QT_END_NAMESPACE

#endif // QQUICKRENDERTARGET_H
