package com.jookkit.handler;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import com.jookkit.conn.ConnectionConfig;
import com.jookkit.conn.ConnectionRegistry;
import com.jookkit.server.JookException;
import java.sql.Connection;
import java.sql.SQLException;

public final class ConnectionHandlers {
    private ConnectionHandlers() {}

    private static final Gson GSON = new Gson();

    /** OPEN_CONNECTION:从 req 解析 ConnectionConfig 并打开。 */
    public static final class Open implements Handler {
        private final ConnectionRegistry registry;
        public Open(ConnectionRegistry registry) { this.registry = registry; }

        @Override
        public JsonObject handle(JsonObject req) {
            ConnectionConfig cfg = GSON.fromJson(req, ConnectionConfig.class);
            registry.open(cfg);
            JsonObject data = new JsonObject();
            data.addProperty("connId", cfg.connId);
            data.addProperty("opened", true);
            return data;
        }
    }

    /** TEST_CONNECTION:打开后立刻关闭,只验证可连。 */
    public static final class Test implements Handler {
        @Override
        public JsonObject handle(JsonObject req) {
            ConnectionConfig cfg = GSON.fromJson(req, ConnectionConfig.class);
            ConnectionRegistry tmp = new ConnectionRegistry();
            try {
                tmp.open(cfg);
                try (Connection c = tmp.get(cfg.connId)) {
                    if (!c.isValid(5)) throw new JookException("CONN_ERROR", "connection invalid");
                }
            } catch (SQLException e) {
                throw new JookException("CONN_ERROR", e.getMessage(), e.getSQLState());
            } finally {
                tmp.close(cfg.connId);
            }
            JsonObject data = new JsonObject();
            data.addProperty("ok", true);
            return data;
        }
    }

    /** CLOSE_CONNECTION。 */
    public static final class Close implements Handler {
        private final ConnectionRegistry registry;
        public Close(ConnectionRegistry registry) { this.registry = registry; }

        @Override
        public JsonObject handle(JsonObject req) {
            registry.close(req.get("connId").getAsString());
            JsonObject data = new JsonObject();
            data.addProperty("closed", true);
            return data;
        }
    }
}
