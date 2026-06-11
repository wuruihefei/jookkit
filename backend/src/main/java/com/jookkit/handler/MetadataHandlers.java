package com.jookkit.handler;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.jookkit.conn.ConnectionRegistry;
import com.jookkit.dialect.Dialect;
import com.jookkit.dialect.Dialects;
import com.jookkit.server.JookException;
import java.sql.Connection;
import java.sql.DatabaseMetaData;
import java.sql.ResultSet;
import java.sql.SQLException;

public final class MetadataHandlers {
    private MetadataHandlers() {}

    private static boolean isSqlite(Connection c) throws SQLException {
        return c.getMetaData().getDatabaseProductName().toLowerCase().contains("sqlite");
    }

    private static Dialect dialectFor(Connection c) throws SQLException {
        return isSqlite(c) ? Dialects.of("sqlite") : Dialects.of("mysql");
    }

    public static final class ListDatabases implements Handler {
        private final ConnectionRegistry registry;
        public ListDatabases(ConnectionRegistry registry) { this.registry = registry; }

        @Override
        public JsonObject handle(JsonObject req) {
            Connection c = registry.get(req.get("connId").getAsString());
            try {
                JsonArray arr = new JsonArray();
                for (String d : dialectFor(c).listDatabases(c)) arr.add(d);
                JsonObject data = new JsonObject();
                data.add("databases", arr);
                return data;
            } catch (SQLException e) {
                throw new JookException("SQL_ERROR", e.getMessage(), e.getSQLState());
            }
        }
    }

    public static final class ListTables implements Handler {
        private final ConnectionRegistry registry;
        public ListTables(ConnectionRegistry registry) { this.registry = registry; }

        @Override
        public JsonObject handle(JsonObject req) {
            Connection c = registry.get(req.get("connId").getAsString());
            String db = req.has("db") ? req.get("db").getAsString() : null;
            try {
                JsonArray arr = new JsonArray();
                for (String t : dialectFor(c).listTables(c, db)) arr.add(t);
                JsonObject data = new JsonObject();
                data.add("tables", arr);
                return data;
            } catch (SQLException e) {
                throw new JookException("SQL_ERROR", e.getMessage(), e.getSQLState());
            }
        }
    }

    /** 结构化读取表 schema:列(含默认值/主键标记)+ 主键 + 索引。 */
    public static final class GetSchema implements Handler {
        private final ConnectionRegistry registry;
        public GetSchema(ConnectionRegistry registry) { this.registry = registry; }

        @Override
        public JsonObject handle(JsonObject req) {
            Connection c = registry.get(req.get("connId").getAsString());
            String table = req.get("table").getAsString();
            try {
                String catalog = (!isSqlite(c) && req.has("db")) ? req.get("db").getAsString() : null;
                DatabaseMetaData md = c.getMetaData();

                java.util.Set<String> pkSet = new java.util.LinkedHashSet<>();
                try (ResultSet rs = md.getPrimaryKeys(catalog, null, table)) {
                    while (rs.next()) pkSet.add(rs.getString("COLUMN_NAME"));
                }

                JsonArray cols = new JsonArray();
                try (ResultSet rs = md.getColumns(catalog, null, table, null)) {
                    while (rs.next()) {
                        JsonObject col = new JsonObject();
                        String name = rs.getString("COLUMN_NAME");
                        col.addProperty("name", name);
                        col.addProperty("type", rs.getString("TYPE_NAME"));
                        col.addProperty("nullable",
                                rs.getInt("NULLABLE") == DatabaseMetaData.columnNullable);
                        col.addProperty("defaultValue", rs.getString("COLUMN_DEF"));
                        col.addProperty("pk", pkSet.contains(name));
                        cols.add(col);
                    }
                }

                JsonArray pks = new JsonArray();
                for (String p : pkSet) pks.add(p);

                // 索引:按索引名分组,收集列
                java.util.LinkedHashMap<String, JsonObject> idxMap = new java.util.LinkedHashMap<>();
                java.util.LinkedHashMap<String, JsonArray> idxCols = new java.util.LinkedHashMap<>();
                try (ResultSet rs = md.getIndexInfo(catalog, null, table, false, false)) {
                    while (rs.next()) {
                        String iname = rs.getString("INDEX_NAME");
                        if (iname == null) continue;  // 统计行
                        String colName = rs.getString("COLUMN_NAME");
                        if (!idxMap.containsKey(iname)) {
                            JsonObject idx = new JsonObject();
                            idx.addProperty("name", iname);
                            idx.addProperty("unique", !rs.getBoolean("NON_UNIQUE"));
                            idxMap.put(iname, idx);
                            idxCols.put(iname, new JsonArray());
                        }
                        if (colName != null) idxCols.get(iname).add(colName);
                    }
                }
                JsonArray indexes = new JsonArray();
                for (var e : idxMap.entrySet()) {
                    e.getValue().add("columns", idxCols.get(e.getKey()));
                    indexes.add(e.getValue());
                }

                JsonObject data = new JsonObject();
                data.add("columns", cols);
                data.add("primaryKeys", pks);
                data.add("indexes", indexes);
                return data;
            } catch (SQLException e) {
                throw new JookException("SQL_ERROR", e.getMessage(), e.getSQLState());
            }
        }
    }

    /** 列出数据库用户。仅 MySQL 支持;其它返回 supported=false。 */
    public static final class ListUsers implements Handler {
        private final ConnectionRegistry registry;
        public ListUsers(ConnectionRegistry registry) { this.registry = registry; }

        @Override
        public JsonObject handle(JsonObject req) {
            Connection c = registry.get(req.get("connId").getAsString());
            JsonObject data = new JsonObject();
            JsonArray users = new JsonArray();
            try {
                if (isSqlite(c)) {
                    data.addProperty("supported", false);
                    data.add("users", users);
                    return data;
                }
                try (var st = c.createStatement();
                     ResultSet rs = st.executeQuery(
                         "SELECT user, host FROM mysql.user ORDER BY user, host")) {
                    while (rs.next()) {
                        JsonObject u = new JsonObject();
                        u.addProperty("user", rs.getString(1));
                        u.addProperty("host", rs.getString(2));
                        users.add(u);
                    }
                }
                data.addProperty("supported", true);
                data.add("users", users);
                return data;
            } catch (SQLException e) {
                throw new JookException("SQL_ERROR", e.getMessage(), e.getSQLState());
            }
        }
    }

    public static final class GetDdl implements Handler {
        private final ConnectionRegistry registry;
        public GetDdl(ConnectionRegistry registry) { this.registry = registry; }

        @Override
        public JsonObject handle(JsonObject req) {
            Connection c = registry.get(req.get("connId").getAsString());
            String table = req.get("table").getAsString();
            try {
                String ddl;
                if (isSqlite(c)) {
                    try (var ps = c.prepareStatement(
                            "SELECT sql FROM sqlite_master WHERE name = ?")) {
                        ps.setString(1, table);
                        try (ResultSet rs = ps.executeQuery()) {
                            ddl = rs.next() ? rs.getString(1) : "";
                        }
                    }
                } else {
                    Dialect d = Dialects.of("mysql");
                    String qualified = req.has("db")
                            ? d.quote(req.get("db").getAsString()) + "." + d.quote(table)
                            : d.quote(table);
                    try (var st = c.createStatement();
                         ResultSet rs = st.executeQuery("SHOW CREATE TABLE " + qualified)) {
                        ddl = rs.next() ? rs.getString(2) : "";
                    }
                }
                JsonObject data = new JsonObject();
                data.addProperty("ddl", ddl);
                return data;
            } catch (SQLException e) {
                throw new JookException("SQL_ERROR", e.getMessage(), e.getSQLState());
            }
        }
    }

    public static final class DescribeTable implements Handler {
        private final ConnectionRegistry registry;
        public DescribeTable(ConnectionRegistry registry) { this.registry = registry; }

        @Override
        public JsonObject handle(JsonObject req) {
            Connection c = registry.get(req.get("connId").getAsString());
            String table = req.get("table").getAsString();
            try {
                // SQLite 忽略 catalog;MySQL 用 db 作 catalog 定位。
                String catalog = (!isSqlite(c) && req.has("db")) ? req.get("db").getAsString() : null;
                DatabaseMetaData md = c.getMetaData();
                JsonArray cols = new JsonArray();
                try (ResultSet rs = md.getColumns(catalog, null, table, null)) {
                    while (rs.next()) {
                        JsonObject col = new JsonObject();
                        col.addProperty("name", rs.getString("COLUMN_NAME"));
                        col.addProperty("type", rs.getString("TYPE_NAME"));
                        col.addProperty("nullable",
                                rs.getInt("NULLABLE") == DatabaseMetaData.columnNullable);
                        cols.add(col);
                    }
                }
                JsonArray pks = new JsonArray();
                try (ResultSet rs = md.getPrimaryKeys(catalog, null, table)) {
                    while (rs.next()) pks.add(rs.getString("COLUMN_NAME"));
                }
                JsonObject data = new JsonObject();
                data.add("columns", cols);
                data.add("primaryKeys", pks);
                return data;
            } catch (SQLException e) {
                throw new JookException("SQL_ERROR", e.getMessage(), e.getSQLState());
            }
        }
    }
}
