package com.jookkit.dialect;

import java.sql.Connection;
import java.sql.SQLException;
import java.util.List;

public interface Dialect {
    /** 列出数据库/schema 名。SQLite 固定返回 ["main"]。 */
    List<String> listDatabases(Connection c) throws SQLException;

    /** 列出某库下的表名。 */
    List<String> listTables(Connection c, String db) throws SQLException;

    /** 标识符引用(MySQL 反引号,SQLite/标准双引号)。 */
    String quote(String identifier);
}
