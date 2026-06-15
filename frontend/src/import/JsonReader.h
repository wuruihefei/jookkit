#ifndef JOOKKIT_JSONREADER_H
#define JOOKKIT_JSONREADER_H

#include <QByteArray>
#include "import/ParseResult.h"

// JSON 解析:顶层须为对象数组 [{...},{...}]。
// headers = 所有对象键的并集(按首次出现顺序);
// null / 缺键 → QString()(NULL);数字/布尔 → 文本形式。
namespace JsonReader {
    ParseResult read(const QByteArray &bytes);
}

#endif
