/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QMLLINTSUGGESTIONS_H
#define QMLLINTSUGGESTIONS_H

#include "qlanguageserver.h"
#include "qqmlcodemodel.h"

#include <optional>

QT_BEGIN_NAMESPACE
namespace QmlLsp {
struct LastLintUpdate
{
    std::optional<int> version;
    std::optional<QDateTime> invalidUpdatesSince;
};

class QmlLintSuggestions : public QLanguageServerModule
{
    Q_OBJECT
public:
    QmlLintSuggestions(QLanguageServer *server, QmlLsp::QQmlCodeModel *codeModel);

    QString name() const override { return QLatin1StringView("QmlLint Suggestions"); }
public slots:
    void diagnose(const QByteArray &uri);
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &) override;

private:
    QMutex m_mutex;
    QHash<QByteArray, LastLintUpdate> m_lastUpdate;
    QLanguageServer *m_server;
    QmlLsp::QQmlCodeModel *m_codeModel;
};
} // namespace QmlLsp
QT_END_NAMESPACE
#endif // QMLLINTSUGGESTIONS_H
