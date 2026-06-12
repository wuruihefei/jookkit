package com.jookkit.handler;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import com.jookkit.conn.ConnectionRegistry;
import com.jookkit.dialect.Dialect;
import com.jookkit.dialect.Dialects;
import com.jookkit.server.JookException;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public final class DataEditHandlers {
    private DataEditHandlers() {}

    private static Dialect dialectFor(Connection c) throws SQLException {
        String product = c.getMetaData().getDatabaseProductName().toLowerCase();
        return product.contains("sqlite") ? Dialects.of("sqlite") : Dialects.of("mysql");
    }

    /** 把 JsonElement 设到 PreparedStatement;NULL 用 setObject(null)。 */
    private static void bind(PreparedStatement ps, int idx, JsonElement el) throws SQLException {
        if (el == null || el.isJsonNull()) { ps.setObject(idx, null); return; }
        JsonPrimitive p = el.getAsJsonPrimitive();
        if (p.isNumber()) ps.setObject(idx, p.getAsBigDecimal());
        else if (p.isBoolean()) ps.setBoolean(idx, p.getAsBoolean());
        else ps.setString(idx, p.getAsString());
    }

    /** 表名限定:请求带非空 db 时加库名前缀,不依赖连接默认库(否则未选库报 1046,或误写其它库同名表)。 */
    static String qualifiedTable(Dialect d, JsonObject req) {
        String table = req.get("table").getAsString();
        if (req.has("db") && !req.get("db").isJsonNull()) {
            String db = req.get("db").getAsString();
            if (!db.isEmpty()) return d.quote(db) + "." + d.quote(table);
        }
        return d.quote(table);
    }

    private static int run(ConnectionRegistry reg, String connId, String sql, List<JsonElement> binds) {
        Connection c = reg.get(connId);
        try (PreparedStatement ps = c.prepareStatement(sql)) {
            for (int i = 0; i < binds.size(); i++) bind(ps, i + 1, binds.get(i));
            return ps.executeUpdate();
        } catch (SQLException e) {
            throw new JookException("SQL_ERROR", e.getMessage(), e.getSQLState());
        }
    }

    private static JsonObject affected(int n) {
        JsonObject data = new JsonObject();
        data.addProperty("affected", n);
        return data;
    }

    public static final class Insert implements Handler {
        private final ConnectionRegistry reg;
        public Insert(ConnectionRegistry reg) { this.reg = reg; }

        @Override
        public JsonObject handle(JsonObject req) {
            Connection c = reg.get(req.get("connId").getAsString());
            try {
                Dialect d = dialectFor(c);
                JsonObject values = req.getAsJsonObject("values");
                List<String> cols = new ArrayList<>();
                List<String> qs = new ArrayList<>();
                List<JsonElement> binds = new ArrayList<>();
                for (Map.Entry<String, JsonElement> e : values.entrySet()) {
                    cols.add(d.quote(e.getKey()));
                    qs.add("?");
                    binds.add(e.getValue());
                }
                String sql = "INSERT INTO " + qualifiedTable(d, req) + " ("
                        + String.join(",", cols) + ") VALUES (" + String.join(",", qs) + ")";
                return affected(run(reg, req.get("connId").getAsString(), sql, binds));
            } catch (SQLException e) {
                throw new JookException("SQL_ERROR", e.getMessage(), e.getSQLState());
            }
        }
    }

    public static final class Update implements Handler {
        private final ConnectionRegistry reg;
        public Update(ConnectionRegistry reg) { this.reg = reg; }

        @Override
        public JsonObject handle(JsonObject req) {
            Connection c = reg.get(req.get("connId").getAsString());
            try {
                Dialect d = dialectFor(c);
                JsonObject values = req.getAsJsonObject("values");
                JsonObject pk = req.getAsJsonObject("pk");
                List<String> sets = new ArrayList<>();
                List<JsonElement> binds = new ArrayList<>();
                for (Map.Entry<String, JsonElement> e : values.entrySet()) {
                    sets.add(d.quote(e.getKey()) + "=?");
                    binds.add(e.getValue());
                }
                List<String> where = new ArrayList<>();
                for (Map.Entry<String, JsonElement> e : pk.entrySet()) {
                    where.add(d.quote(e.getKey()) + "=?");
                    binds.add(e.getValue());
                }
                String sql = "UPDATE " + qualifiedTable(d, req) + " SET " + String.join(",", sets)
                        + " WHERE " + String.join(" AND ", where);
                return affected(run(reg, req.get("connId").getAsString(), sql, binds));
            } catch (SQLException e) {
                throw new JookException("SQL_ERROR", e.getMessage(), e.getSQLState());
            }
        }
    }

    public static final class Delete implements Handler {
        private final ConnectionRegistry reg;
        public Delete(ConnectionRegistry reg) { this.reg = reg; }

        @Override
        public JsonObject handle(JsonObject req) {
            Connection c = reg.get(req.get("connId").getAsString());
            try {
                Dialect d = dialectFor(c);
                JsonObject pk = req.getAsJsonObject("pk");
                List<String> where = new ArrayList<>();
                List<JsonElement> binds = new ArrayList<>();
                for (Map.Entry<String, JsonElement> e : pk.entrySet()) {
                    where.add(d.quote(e.getKey()) + "=?");
                    binds.add(e.getValue());
                }
                String sql = "DELETE FROM " + qualifiedTable(d, req)
                        + " WHERE " + String.join(" AND ", where);
                return affected(run(reg, req.get("connId").getAsString(), sql, binds));
            } catch (SQLException e) {
                throw new JookException("SQL_ERROR", e.getMessage(), e.getSQLState());
            }
        }
    }
}
