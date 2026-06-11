package com.jookkit.server;

import com.google.gson.JsonObject;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class ProtocolTest {
    @Test
    void okWrapsData() {
        JsonObject data = new JsonObject();
        data.addProperty("x", 1);
        JsonObject r = Protocol.ok(3001, data);
        assertEquals(3001, r.get("funcId").getAsInt());
        assertTrue(r.get("ok").getAsBoolean());
        assertEquals(1, r.getAsJsonObject("data").get("x").getAsInt());
    }

    @Test
    void errorCarriesCodeAndSqlState() {
        JsonObject r = Protocol.error(3001, "SQL_ERROR", "boom", "42S02");
        assertFalse(r.get("ok").getAsBoolean());
        JsonObject err = r.getAsJsonObject("error");
        assertEquals("SQL_ERROR", err.get("code").getAsString());
        assertEquals("boom", err.get("message").getAsString());
        assertEquals("42S02", err.get("sqlState").getAsString());
    }

    @Test
    void errorOmitsNullSqlState() {
        JsonObject r = Protocol.error(1, "X", "m", null);
        assertFalse(r.getAsJsonObject("error").has("sqlState"));
    }
}
