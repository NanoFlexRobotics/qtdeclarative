/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

import QtQuick 2.0

Row {
    id: root
    width: 320
    height: 480
    Flickable {
        id: fl1
        objectName: "fl1"
        width: parent.width / 2
        height: parent.height
        contentHeight: 640
        Rectangle {
            width: fl1.width
            height: 640
            color: fl1.dragging ? "steelblue" : "lightsteelblue"
            Text {
                anchors.centerIn: parent
                text: "flick this"
            }
        }
    }
    Flickable {
        id: fl2
        objectName: "fl2"
        width: parent.width / 2
        height: parent.height
        contentHeight: 640
        Rectangle {
            width: fl2.width
            height: 640
            color: fl2.dragging ? "bisque" : "beige"
            Text {
                anchors.centerIn: parent
                text: "and flick\nthis too"
            }
        }
    }
}