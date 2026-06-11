package com.jookkit.server;

public final class FuncId {
    private FuncId() {}

    public static final int TEST_CONNECTION = 1001;
    public static final int OPEN_CONNECTION = 1002;
    public static final int CLOSE_CONNECTION = 1003;

    public static final int LIST_DATABASES = 2001;
    public static final int LIST_TABLES = 2002;
    public static final int DESCRIBE_TABLE = 2003;

    public static final int EXEC_SQL = 3001;

    public static final int INSERT_ROW = 4001;
    public static final int UPDATE_ROW = 4002;
    public static final int DELETE_ROW = 4003;
}
