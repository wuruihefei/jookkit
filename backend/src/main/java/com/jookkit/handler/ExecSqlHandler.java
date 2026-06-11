package com.jookkit.handler;

import com.google.gson.JsonObject;
import com.jookkit.codec.ResultSetCodec;
import com.jookkit.conn.ConnectionRegistry;
import com.jookkit.server.JookException;
import java.sql.Connection;
import java.sql.SQLException;
import java.sql.Statement;

public final class ExecSqlHandler implements Handler {
    private final ConnectionRegistry registry;

    public ExecSqlHandler(ConnectionRegistry registry) {
        this.registry = registry;
    }

    @Override
    public JsonObject handle(JsonObject req) {
        String connId = req.get("connId").getAsString();
        String sql = req.get("sql").getAsString();
        Connection c = registry.get(connId);
        try (Statement st = c.createStatement()) {
            boolean isQuery = st.execute(sql);
            JsonObject data;
            if (isQuery) {
                data = ResultSetCodec.encode(st.getResultSet());
                data.addProperty("isQuery", true);
            } else {
                data = new JsonObject();
                data.addProperty("isQuery", false);
                data.addProperty("affected", st.getUpdateCount());
            }
            return data;
        } catch (SQLException e) {
            throw new JookException("SQL_ERROR", e.getMessage(), e.getSQLState());
        }
    }
}
