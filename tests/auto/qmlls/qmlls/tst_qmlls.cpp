/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
#include <QtJsonRpc/private/qjsonrpcprotocol_p.h>
#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qprocess.h>
#include <QtCore/qlibraryinfo.h>

#include <QtTest/qtest.h>

#include <iostream>

using namespace QLspSpecification;

class DiagnosticsHandler
{
public:
    void handleNotification(const PublishDiagnosticsParams &params)
    {
        m_received.append(PublishDiagnosticsParams(params));
    }

    bool contains(const QString &uri, int line1, int column1, int line2, int column2) const
    {
        for (const auto &params : m_received) {
            if (params.uri != uri)
                continue;
            for (const auto &diagnostic : params.diagnostics) {
                const auto range = diagnostic.range;
                if (range.start.line == line1 && range.start.character == column1
                    && range.end.line == line2 && range.end.character == column2) {
                    return true;
                }
            }
        }
        return false;
    }

    int numDiagnostics(const QByteArray &uri) const
    {
        int num = 0;
        for (const auto &params : m_received) {
            if (params.uri == uri)
                num += params.diagnostics.length();
        }
        return num;
    }

    QList<Diagnostic> diagnostics(const QByteArray &uri) const
    {
        QList<Diagnostic> result;
        for (const auto &params : m_received) {
            if (params.uri == uri)
                result << params.diagnostics;
        }

        return result;
    }

    void clear() { m_received.clear(); }

private:
    QList<PublishDiagnosticsParams> m_received;
};

class tst_Qmlls : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_Qmlls();
private slots:
    void initTestCase() final;
    void didOpenTextDocument();
    void cleanupTestCase();

private:
    QProcess m_server;
    QLanguageServerProtocol m_protocol;
    DiagnosticsHandler m_diagnosticsHandler;
    QString m_qmllsPath;
};

tst_Qmlls::tst_Qmlls()
    : QQmlDataTest(QT_QMLTEST_DATADIR),
      m_protocol([this](const QByteArray &data) { m_server.write(data); })
{
    connect(&m_server, &QProcess::readyReadStandardOutput, this, [this]() {
        QByteArray data = m_server.readAllStandardOutput();
        m_protocol.receiveData(data);
    });

    connect(&m_server, &QProcess::readyReadStandardError, this,
            [this]() { qWarning() << "LSPerr" << m_server.readAllStandardError(); });

    m_qmllsPath = QLibraryInfo::path(QLibraryInfo::BinariesPath) + QLatin1String("/qmlls");
#ifdef Q_OS_WIN
    m_qmllsPath += QLatin1String(".exe");
#endif
    m_server.setProgram(m_qmllsPath);
    m_protocol.registerPublishDiagnosticsNotificationHandler(
            [this](const QByteArray &, auto params) {
                m_diagnosticsHandler.handleNotification(params);
            });
}

void tst_Qmlls::initTestCase()
{
    QQmlDataTest::initTestCase();
    if (!QFileInfo::exists(m_qmllsPath)) {
        QString message =
                QStringLiteral("qmlls executable not found (looked for %0)").arg(m_qmllsPath);
        QSKIP(qPrintable(message)); // until we add a feature for this we avoid failing here
    }
    m_server.start();
    InitializeParams clientInfo;
    clientInfo.rootUri = QUrl::fromLocalFile(dataDirectory() + "/default").toString().toUtf8();
    TextDocumentClientCapabilities tDoc;
    PublishDiagnosticsClientCapabilities pDiag;
    tDoc.publishDiagnostics = pDiag;
    pDiag.versionSupport = true;
    clientInfo.capabilities.textDocument = tDoc;
    bool didInit = false;
    m_protocol.requestInitialize(clientInfo, [this, &didInit](const InitializeResult &serverInfo) {
        Q_UNUSED(serverInfo);
        m_protocol.notifyInitialized(InitializedParams());
        didInit = true;
    });
    QTRY_COMPARE_WITH_TIMEOUT(didInit, true, 10000);
}

void tst_Qmlls::didOpenTextDocument()
{
    QFile file(testFile("default/Yyy.qml"));
    QVERIFY(file.open(QIODevice::ReadOnly));

    DidOpenTextDocumentParams params;
    TextDocumentItem textDocument;
    QByteArray uri = testFileUrl("default/Yyy.qml").toString().toUtf8();
    textDocument.uri = uri;
    textDocument.text = file.readAll().replace("width", "wildth");
    params.textDocument = textDocument;
    m_protocol.notifyDidOpenTextDocument(params);

    QTRY_VERIFY_WITH_TIMEOUT(m_diagnosticsHandler.numDiagnostics(uri) != 0, 10000);
    QVERIFY(m_diagnosticsHandler.contains(uri, 3, 4, 3, 10));

    auto diagnostics = m_diagnosticsHandler.diagnostics(uri);

    CodeActionParams codeActionParams;
    codeActionParams.textDocument = { textDocument.uri };
    codeActionParams.context.diagnostics = diagnostics;
    codeActionParams.range.start = Position { 0, 0 };
    codeActionParams.range.end =
            Position { static_cast<int>(textDocument.text.split(u'\n').size()), 0 };

    bool success = false;
    m_protocol.requestCodeAction(
            codeActionParams,
            [&](const std::variant<QList<std::variant<Command, CodeAction>>, std::nullptr_t>
                        &response) {
                using ListType = QList<std::variant<Command, CodeAction>>;

                QVERIFY(std::holds_alternative<ListType>(response));

                auto list = std::get<ListType>(response);

                QList<QPair<QString, QString>> expectedData = {
                    { QLatin1StringView("Did you mean \"width\"?"), QLatin1StringView("width") },
                    { QLatin1StringView("Did you mean \"z\"?"), QLatin1StringView("z") }
                };
                QCOMPARE(list.size(), expectedData.size());

                for (const auto &entry : list) {
                    QVERIFY(std::holds_alternative<CodeAction>(entry));
                    CodeAction action = std::get<CodeAction>(entry);

                    QString title = QString::fromUtf8(action.title);
                    QVERIFY(action.kind.has_value());
                    QCOMPARE(QString::fromUtf8(action.kind.value()),
                             QLatin1StringView("refactor.rewrite"));
                    QVERIFY(action.edit.has_value());
                    WorkspaceEdit edit = action.edit.value();

                    QVERIFY(edit.documentChanges.has_value());
                    auto docChangeVariant = edit.documentChanges.value();
                    QVERIFY(std::holds_alternative<QList<TextDocumentEdit>>(docChangeVariant));
                    auto documentChanges = std::get<QList<TextDocumentEdit>>(docChangeVariant);
                    QCOMPARE(documentChanges.size(), 1);

                    TextDocumentEdit textDocEdit = documentChanges.first();
                    QCOMPARE(textDocEdit.textDocument.uri, textDocument.uri);

                    QCOMPARE(textDocEdit.edits.size(), 1);
                    auto editVariant = textDocEdit.edits.first();
                    QVERIFY(std::holds_alternative<TextEdit>(editVariant));

                    TextEdit textEdit = std::get<TextEdit>(editVariant);
                    QString newText = QString::fromUtf8(textEdit.newText);
                    QPair<QString, QString> data = { title, newText };

                    qsizetype dataIndex = expectedData.indexOf(data);
                    QVERIFY2(dataIndex != -1,
                             qPrintable(QLatin1String("{\"%1\",\"%2\"}").arg(title, newText)));
                    // Make sure every expected entry only occurs once
                    expectedData.remove(dataIndex);
                }

                success = true;
            },
            [](const QLspSpecification::ResponseError &error) {
                qWarning() << "CodeAction Error:" << QString::fromUtf8(error.message);
            });

    QTRY_VERIFY_WITH_TIMEOUT(success, 10000);
    m_diagnosticsHandler.clear();
}

void tst_Qmlls::cleanupTestCase()
{
    m_server.closeWriteChannel();
    QTRY_COMPARE(m_server.state(), QProcess::NotRunning);
    QCOMPARE(m_server.exitStatus(), QProcess::NormalExit);
}

QTEST_MAIN(tst_Qmlls)

#include <tst_qmlls.moc>
