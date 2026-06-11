package com.jookkit.handler;

import com.google.gson.JsonObject;
import com.jookkit.conn.ConnectionConfig;
import com.jookkit.conn.ConnectionRegistry;
import com.jookkit.server.JookException;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class ExecSqlHandlerTest {
    private ConnectionRegistry regWith(String id) {
        ConnectionRegistry reg = new ConnectionRegistry();
        ConnectionConfig c = new ConnectionConfig();
        c.connId = id; c.type = "sqlite"; c.file = ":memory:";
        reg.open(c);
        return reg;
    }

    private JsonObject req(String connId, String sql) {
        JsonObject o = new JsonObject();
        o.addProperty("connId", connId);
        o.addProperty("sql", sql);
        return o;
    }

    @Test
    void selectReturnsColumnsAndRows() {
        ConnectionRegistry reg = regWith("c1");
        ExecSqlHandler h = new ExecSqlHandler(reg);
        h.handle(req("c1", "create table t(id integer, name text)"));
        h.handle(req("c1", "insert into t values (1,'a')"));
        JsonObject data = h.handle(req("c1", "select * from t"));
        assertTrue(data.get("isQuery").getAsBoolean());
        assertEquals(1, data.getAsJsonArray("rows").size());
    }

    @Test
    void updateReturnsAffected() {
        ConnectionRegistry reg = regWith("c1");
        ExecSqlHandler h = new ExecSqlHandler(reg);
        h.handle(req("c1", "create table t(id integer)"));
        h.handle(req("c1", "insert into t values (1),(2)"));
        JsonObject data = h.handle(req("c1", "delete from t"));
        assertFalse(data.get("isQuery").getAsBoolean());
        assertEquals(2, data.get("affected").getAsInt());
    }

    @Test
    void sqlErrorThrowsJookException() {
        ConnectionRegistry reg = regWith("c1");
        ExecSqlHandler h = new ExecSqlHandler(reg);
        JookException ex = assertThrows(JookException.class,
                () -> h.handle(req("c1", "select * from nope")));
        assertEquals("SQL_ERROR", ex.code);
    }
}
