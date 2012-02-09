/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "profileclient.h"

#include <QtCore/QStack>
#include <QtCore/QStringList>

ProfileClient::ProfileClient(const QString &clientName,
                             QDeclarativeDebugConnection *client)
    : QDeclarativeDebugClient(clientName, client),
      m_recording(false),
      m_enabled(false)
{
}

ProfileClient::~ProfileClient()
{
    //Disable profiling if started by client
    //Profiling data will be lost!!
    if (isRecording())
        setRecording(false);
}

void ProfileClient::clearData()
{
    emit cleared();
}

bool ProfileClient::isEnabled() const
{
    return m_enabled;
}

void ProfileClient::sendRecordingStatus()
{
}

bool ProfileClient::isRecording() const
{
    return m_recording;
}

void ProfileClient::setRecording(bool v)
{
    if (v == m_recording)
        return;

    m_recording = v;

    if (state() == Enabled) {
        sendRecordingStatus();
    }

    emit recordingChanged(v);
}

void ProfileClient::stateChanged(State status)
{
    if ((m_enabled && status != Enabled) ||
            (!m_enabled && status == Enabled))
        emit enabledChanged();

    m_enabled = status == Enabled;

}

class DeclarativeProfileClientPrivate
{
public:
    DeclarativeProfileClientPrivate()
        : inProgressRanges(0)
        , maximumTime(0)
    {
        ::memset(rangeCount, 0,
                 QDeclarativeProfilerService::MaximumRangeType * sizeof(int));
    }

    qint64 inProgressRanges;
    QStack<qint64> rangeStartTimes[QDeclarativeProfilerService::MaximumRangeType];
    QStack<QStringList> rangeDatas[QDeclarativeProfilerService::MaximumRangeType];
    QStack<EventLocation> rangeLocations[QDeclarativeProfilerService::MaximumRangeType];
    int rangeCount[QDeclarativeProfilerService::MaximumRangeType];
    qint64 maximumTime;
};

DeclarativeProfileClient::DeclarativeProfileClient(
        QDeclarativeDebugConnection *client)
    : ProfileClient(QLatin1String("CanvasFrameRate"), client),
      d(new DeclarativeProfileClientPrivate)
{
}

DeclarativeProfileClient::~DeclarativeProfileClient()
{
    delete d;
}

void DeclarativeProfileClient::clearData()
{
    ::memset(d->rangeCount, 0,
             QDeclarativeProfilerService::MaximumRangeType * sizeof(int));
    ProfileClient::clearData();
}

void DeclarativeProfileClient::sendRecordingStatus()
{
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream << isRecording();
    sendMessage(ba);
}

void DeclarativeProfileClient::messageReceived(const QByteArray &data)
{
    QByteArray rwData = data;
    QDataStream stream(&rwData, QIODevice::ReadOnly);

    qint64 time;
    int messageType;

    stream >> time >> messageType;

    if (messageType >= QDeclarativeProfilerService::MaximumMessage)
        return;

    if (messageType == QDeclarativeProfilerService::Event) {
        int event;
        stream >> event;

        if (event == QDeclarativeProfilerService::EndTrace) {
            emit this->traceFinished(time);
            d->maximumTime = time;
            d->maximumTime = qMax(time, d->maximumTime);
        } else if (event == QDeclarativeProfilerService::AnimationFrame) {
            int frameRate, animationCount;
            stream >> frameRate >> animationCount;
            emit this->frame(time, frameRate, animationCount);
            d->maximumTime = qMax(time, d->maximumTime);
        } else if (event == QDeclarativeProfilerService::StartTrace) {
            emit this->traceStarted(time);
            d->maximumTime = time;
        } else if (event < QDeclarativeProfilerService::MaximumEventType) {
            d->maximumTime = qMax(time, d->maximumTime);
        }
    } else if (messageType == QDeclarativeProfilerService::Complete) {
        emit complete();

    } else {
        int range;
        stream >> range;

        if (range >= QDeclarativeProfilerService::MaximumRangeType)
            return;

        if (messageType == QDeclarativeProfilerService::RangeStart) {
            d->rangeStartTimes[range].push(time);
            d->inProgressRanges |= (static_cast<qint64>(1) << range);
            ++d->rangeCount[range];
        } else if (messageType == QDeclarativeProfilerService::RangeData) {
            QString data;
            stream >> data;

            int count = d->rangeCount[range];
            if (count > 0) {
                while (d->rangeDatas[range].count() < count)
                    d->rangeDatas[range].push(QStringList());
                d->rangeDatas[range][count-1] << data;
            }

        } else if (messageType == QDeclarativeProfilerService::RangeLocation) {
            QString fileName;
            int line;
            int column = -1;
            stream >> fileName >> line;

            if (!stream.atEnd())
                stream >> column;

            if (d->rangeCount[range] > 0) {
                d->rangeLocations[range].push(EventLocation(fileName, line,
                                                            column));
            }
        } else {
            if (d->rangeCount[range] > 0) {
                --d->rangeCount[range];
                if (d->inProgressRanges & (static_cast<qint64>(1) << range))
                    d->inProgressRanges &= ~(static_cast<qint64>(1) << range);

                d->maximumTime = qMax(time, d->maximumTime);
                QStringList data = d->rangeDatas[range].count() ?
                            d->rangeDatas[range].pop() : QStringList();
                EventLocation location = d->rangeLocations[range].count() ?
                            d->rangeLocations[range].pop() : EventLocation();

                qint64 startTime = d->rangeStartTimes[range].pop();
                emit this->range((QDeclarativeProfilerService::RangeType)range,
                                 startTime, time - startTime, data, location);
                if (d->rangeCount[range] == 0) {
                    int count = d->rangeDatas[range].count() +
                                d->rangeStartTimes[range].count() +
                                d->rangeLocations[range].count();
                    if (count != 0)
                        qWarning() << "incorrectly nested data";
                }
            }
        }
    }
}

V8ProfileClient::V8ProfileClient(QDeclarativeDebugConnection *client)
    : ProfileClient(QLatin1String("V8Profiler"), client)
{
}

V8ProfileClient::~V8ProfileClient()
{
}

void V8ProfileClient::sendRecordingStatus()
{
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    QByteArray cmd("V8PROFILER");
    QByteArray option("");
    QByteArray title("");

    if (m_recording) {
        option = "start";
    } else {
        option = "stop";
    }
    stream << cmd << option << title;
    sendMessage(ba);
}

void V8ProfileClient::messageReceived(const QByteArray &data)
{
    QByteArray rwData = data;
    QDataStream stream(&rwData, QIODevice::ReadOnly);

    int messageType;

    stream >> messageType;

    if (messageType == V8Complete) {
        emit complete();
    } else if (messageType == V8Entry) {
        QString filename;
        QString function;
        int lineNumber;
        double totalTime;
        double selfTime;
        int depth;

        stream  >> filename >> function >> lineNumber >> totalTime >>
                   selfTime >> depth;
        emit this->range(depth, function, filename, lineNumber, totalTime,
                         selfTime);
    }
}

