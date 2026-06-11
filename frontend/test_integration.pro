QT += core network testlib
CONFIG += c++17 console testcase
TEMPLATE = app
TARGET = tst_integration
INCLUDEPATH += src

SOURCES += \
    tests/tst_backend_integration.cpp \
    src/backend/ConnData.cpp \
    src/backend/BackendProcess.cpp \
    src/backend/BackendClient.cpp

HEADERS += \
    src/backend/ConnData.h \
    src/backend/BackendProcess.h \
    src/backend/BackendClient.h \
    src/backend/FuncId.h
