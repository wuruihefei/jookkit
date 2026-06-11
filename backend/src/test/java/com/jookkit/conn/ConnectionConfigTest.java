package com.jookkit.conn;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class ConnectionConfigTest {
    @Test
    void buildsSqliteUrl() {
        ConnectionConfig c = new ConnectionConfig();
        c.type = "sqlite";
        c.file = "/tmp/x.db";
        assertEquals("jdbc:sqlite:/tmp/x.db", c.jdbcUrl());
    }

    @Test
    void buildsMysqlUrl() {
        ConnectionConfig c = new ConnectionConfig();
        c.type = "mysql";
        c.host = "localhost";
        c.port = 3306;
        c.database = "test";
        assertEquals("jdbc:mysql://localhost:3306/test", c.jdbcUrl());
    }
}
