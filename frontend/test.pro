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
    src/ui/GridUtils.cpp \
    src/import/CsvReader.cpp \
    src/import/JsonReader.cpp \
    src/import/ColumnMapping.cpp \
    src/import/ImportRunner.cpp

HEADERS += \
    src/backend/ConnData.h \
    src/backend/BackendClient.h \
    src/backend/BackendProcess.h \
    src/backend/FuncId.h \
    src/export/Exporter.h \
    src/store/HistoryStore.h \
    src/ui/GridUtils.h \
    src/import/ParseResult.h \
    src/import/CsvReader.h \
    src/import/JsonReader.h \
    src/import/ColumnMapping.h \
    src/import/ImportRunner.h

SOURCES += \
    src/store/HistoryStore.cpp
