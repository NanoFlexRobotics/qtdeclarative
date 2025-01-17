/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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

#ifndef QQUICKIOSSTYLE_P_H
#define QQUICKIOSSTYLE_P_H

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

class QQuickIOSStyle : public QQuickAttachedObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url CONSTANT)
    Q_PROPERTY(Theme theme READ theme WRITE setTheme NOTIFY themeChanged FINAL)
    QML_NAMED_ELEMENT(IOS)
    QML_ATTACHED(QQuickIOSStyle)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(2, 3)

public:
    enum Theme {
        Light,
        Dark
    };
    Q_ENUM(Theme)

    explicit QQuickIOSStyle(QObject *parent = nullptr);

    static QQuickIOSStyle *qmlAttachedProperties(QObject *object);

    Theme theme() const;
    void setTheme(Theme theme);

    QUrl url() const;

Q_SIGNALS:
    void themeChanged();

private:
    void init();

    Theme m_theme = Light;
};

QT_END_NAMESPACE

QML_DECLARE_TYPEINFO(QQuickIOSStyle, QML_HAS_ATTACHED_PROPERTIES)

#endif // QQUICKIOSSTYLE_P_H
