QT += core gui widgets network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17
TARGET = jookkit
TEMPLATE = app

INCLUDEPATH += src

SOURCES += \
    src/main.cpp \
    src/ui/MinimalWindow.cpp \
    src/ui/SqlEditor.cpp \
    src/sql/SqlSplitter.cpp \
    src/sql/SqlHighlighter.cpp \
    src/backend/ConnData.cpp \
    src/backend/BackendProcess.cpp \
    src/backend/BackendClient.cpp

HEADERS += \
    src/ui/MinimalWindow.h \
    src/ui/SqlEditor.h \
    src/sql/SqlSplitter.h \
    src/sql/SqlHighlighter.h \
    src/backend/ConnData.h \
    src/backend/BackendProcess.h \
    src/backend/BackendClient.h \
    src/backend/FuncId.h
