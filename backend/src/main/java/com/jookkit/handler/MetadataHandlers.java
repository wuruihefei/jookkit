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
