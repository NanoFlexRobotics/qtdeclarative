TARGET = qtquicktemplates2plugin
TARGETPATH = QtQuick/Templates
QML_IMPORT_VERSION = $$QT_VERSION

QT += qml quick
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

OTHER_FILES += \
    qmldir

SOURCES += \
    $$PWD/qtquicktemplates2plugin.cpp

CONFIG += no_cxx_module
load(qml_plugin)