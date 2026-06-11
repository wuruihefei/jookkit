QT += core network testlib
CONFIG += c++17 console testcase
TEMPLATE = app
TARGET = tst_jookkit

INCLUDEPATH += src

SOURCES += \
    tests/tst_conndata.cpp \
    src/backend/ConnData.cpp \
    src/backend/BackendClient.cpp \
    src/backend/BackendProcess.cpp

HEADERS += \
    src/backend/ConnData.h \
    src/backend/BackendClient.h \
    src/backend/BackendProcess.h \
    src/backend/FuncId.h
