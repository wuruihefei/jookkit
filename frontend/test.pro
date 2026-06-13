QT += core gui widgets network testlib
CONFIG += c++17 console testcase
TEMPLATE = app
TARGET = tst_jookkit

INCLUDEPATH += src

SOURCES += \
    tests/main_tests.cpp \
    src/backend/ConnData.cpp \
    src/backend/BackendClient.cpp \
    src/backend/BackendProcess.cpp \
    src/export/Exporter.cpp \
    src/ui/GridUtils.cpp

HEADERS += \
    src/backend/ConnData.h \
    src/backend/BackendClient.h \
    src/backend/BackendProcess.h \
    src/backend/FuncId.h \
    src/export/Exporter.h \
    src/store/HistoryStore.h \
    src/ui/GridUtils.h

SOURCES += \
    src/store/HistoryStore.cpp
