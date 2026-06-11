#ifndef JOOKKIT_FUNCID_H
#define JOOKKIT_FUNCID_H

namespace FuncId {
    constexpr int TEST_CONNECTION  = 1001;
    constexpr int OPEN_CONNECTION  = 1002;
    constexpr int CLOSE_CONNECTION = 1003;
    constexpr int LIST_DATABASES   = 2001;
    constexpr int LIST_TABLES      = 2002;
    constexpr int DESCRIBE_TABLE   = 2003;
    constexpr int GET_DDL          = 2004;
    constexpr int EXEC_SQL         = 3001;
    constexpr int INSERT_ROW       = 4001;
    constexpr int UPDATE_ROW       = 4002;
    constexpr int DELETE_ROW       = 4003;
}

#endif
