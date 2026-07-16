#-------------------------------------------------
#
# Project created by QtCreator 2018-04-04T10:31:33
#
#-------------------------------------------------
# 这个注释包含明确的 UTF-8 特征字符：中文测试 áéíóú ñ ç ß
# -*- coding: utf-8 -*-

QT       += core gui
QT       += core gui xml
QT       += network

# MSVC: 强制以UTF-8读取源文件和执行字符集
msvc: QMAKE_CXXFLAGS += /utf-8

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = TSFileEditor
TEMPLATE = app

include(./../../Path.pri)

INCLUDEPATH += ../../Libraries

LIBS += -L$${DESTDIR} -lLibXlsxRW

SOURCES += main.cpp\
        MainWindow.cpp \
    XmlRW.cpp \
    ExcelRW.cpp \
    DataModel/TranslateModel.cpp \
    NetWorker.cpp \
    TranslateWorker.cpp \
    ScriptErrorDialog.cpp \
    HelpDialog.cpp

HEADERS  += MainWindow.h \
    XmlRW.h \
    ExcelRW.h \
    DataModel/TranslateModel.h \
    NetWorker.h \
    TranslateWorker.h \
    ScriptErrorDialog.h \
    HelpDialog.h

FORMS    += MainWindow.ui



