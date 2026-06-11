package com.jookkit.conn;

import com.jookkit.server.JookException;
import org.junit.jupiter.api.Test;
import java.sql.Connection;
import static org.junit.jupiter.api.Assertions.*;

class ConnectionRegistryTest {
    private ConnectionConfig sqliteMem(String id) {
        ConnectionConfig c = new ConnectionConfig();
        c.connId = id;
        c.type = "sqlite";
        c.file = ":memory:";
        return c;
    }

    @Test
    void openThenGetReturnsSameConnection() throws Exception {
        ConnectionRegistry reg = new ConnectionRegistry();
        reg.open(sqliteMem("c1"));
        Connection a = reg.get("c1");
        Connection b = reg.get("c1");
        assertSame(a, b);
        assertFalse(a.isClosed());
        reg.close("c1");
        assertTrue(a.isClosed());
    }

    @Test
    void getUnknownThrows() {
        ConnectionRegistry reg = new ConnectionRegistry();
        assertThrows(JookException.class, () -> reg.get("nope"));
    }
}
