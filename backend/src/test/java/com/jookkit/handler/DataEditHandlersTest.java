package com.jookkit.handler;

import com.google.gson.JsonObject;
import com.jookkit.conn.ConnectionConfig;
import com.jookkit.conn.ConnectionRegistry;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class DataEditHandlersTest {
    private ConnectionRegistry reg;

    @BeforeEach
    void setup() {
        reg = new ConnectionRegistry();
        ConnectionConfig c = new ConnectionConfig();
        c.connId = "c1"; c.type = "sqlite"; c.file = ":memory:";
        reg.open(c);
        JsonObject ddl = new JsonObject();
        ddl.addProperty("connId", "c1");
        ddl.addProperty("sql", "create table t(id integer primary key, name text)");
        new ExecSqlHandler(reg).handle(ddl);
    }

    private JsonObject base(String table) {
        JsonObject o = new JsonObject();
        o.addProperty("connId", "c1");
        o.addProperty("table", table);
        return o;
    }

    private int count() {
        JsonObject q = new JsonObject();
        q.addProperty("connId", "c1");
        q.addProperty("sql", "select count(*) from t");
        return new ExecSqlHandler(reg).handle(q)
                .getAsJsonArray("rows").get(0).getAsJsonArray().get(0).getAsInt();
    }

    @Test
    void insertAddsRow() {
        JsonObject req = base("t");
        JsonObject values = new JsonObject();
        values.addProperty("id", 1);
        values.addProperty("name", "alice");
        req.add("values", values);
        JsonObject data = new DataEditHandlers.Insert(reg).handle(req);
        assertEquals(1, data.get("affected").getAsInt());
        assertEquals(1, count());
    }

    @Test
    void updateChangesRow() {
        JsonObject ins = base("t");
        JsonObject v = new JsonObject(); v.addProperty("id", 1); v.addProperty("name", "a");
        ins.add("values", v);
        new DataEditHandlers.Insert(reg).handle(ins);

        JsonObject req = base("t");
        JsonObject newVals = new JsonObject(); newVals.addProperty("name", "bob");
        JsonObject pk = new JsonObject(); pk.addProperty("id", 1);
        req.add("values", newVals);
        req.add("pk", pk);
        JsonObject data = new DataEditHandlers.Update(reg).handle(req);
        assertEquals(1, data.get("affected").getAsInt());
    }

    @Test
    void deleteRemovesRow() {
        JsonObject ins = base("t");
        JsonObject v = new JsonObject(); v.addProperty("id", 1); v.addProperty("name", "a");
        ins.add("values", v);
        new DataEditHandlers.Insert(reg).handle(ins);

        JsonObject req = base("t");
        JsonObject pk = new JsonObject(); pk.addProperty("id", 1);
        req.add("pk", pk);
        JsonObject data = new DataEditHandlers.Delete(reg).handle(req);
        assertEquals(1, data.get("affected").getAsInt());
        assertEquals(0, count());
    }
}
