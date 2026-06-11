package com.jookkit.server;

import com.google.gson.JsonObject;
import com.jookkit.conn.ConnectionRegistry;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class DispatcherTest {
    @Test
    void routesOpenAndExec() {
        Dispatcher d = new Dispatcher(new ConnectionRegistry());

        JsonObject open = new JsonObject();
        open.addProperty("funcId", FuncId.OPEN_CONNECTION);
        open.addProperty("connId", "c1");
        open.addProperty("type", "sqlite");
        open.addProperty("file", ":memory:");
        JsonObject r1 = d.dispatch(open);
        assertTrue(r1.get("ok").getAsBoolean());

        JsonObject exec = new JsonObject();
        exec.addProperty("funcId", FuncId.EXEC_SQL);
        exec.addProperty("connId", "c1");
        exec.addProperty("sql", "select 1 as one");
        JsonObject r2 = d.dispatch(exec);
        assertTrue(r2.get("ok").getAsBoolean());
        assertEquals(1, r2.getAsJsonObject("data")
                .getAsJsonArray("rows").get(0).getAsJsonArray().get(0).getAsInt());
    }

    @Test
    void unknownFuncIdReturnsError() {
        Dispatcher d = new Dispatcher(new ConnectionRegistry());
        JsonObject req = new JsonObject();
        req.addProperty("funcId", 99999);
        JsonObject r = d.dispatch(req);
        assertFalse(r.get("ok").getAsBoolean());
        assertEquals("UNKNOWN_FUNC", r.getAsJsonObject("error").get("code").getAsString());
    }

    @Test
    void sqlErrorBecomesErrorEnvelope() {
        Dispatcher d = new Dispatcher(new ConnectionRegistry());
        JsonObject open = new JsonObject();
        open.addProperty("funcId", FuncId.OPEN_CONNECTION);
        open.addProperty("connId", "c1");
        open.addProperty("type", "sqlite");
        open.addProperty("file", ":memory:");
        d.dispatch(open);

        JsonObject exec = new JsonObject();
        exec.addProperty("funcId", FuncId.EXEC_SQL);
        exec.addProperty("connId", "c1");
        exec.addProperty("sql", "select * from nope");
        JsonObject r = d.dispatch(exec);
        assertFalse(r.get("ok").getAsBoolean());
        assertEquals("SQL_ERROR", r.getAsJsonObject("error").get("code").getAsString());
    }
}
