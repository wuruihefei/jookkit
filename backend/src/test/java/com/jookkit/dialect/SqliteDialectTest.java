package com.jookkit.dialect;

import org.junit.jupiter.api.Test;
import java.sql.*;
import java.util.List;
import static org.junit.jupiter.api.Assertions.*;

class SqliteDialectTest {
    private Connection mem() throws SQLException {
        return DriverManager.getConnection("jdbc:sqlite::memory:");
    }

    @Test
    void quoteUsesDoubleQuotes() {
        assertEquals("\"col\"", new SqliteDialect().quote("col"));
    }

    @Test
    void listDatabasesReturnsMain() throws SQLException {
        try (Connection c = mem()) {
            assertEquals(List.of("main"), new SqliteDialect().listDatabases(c));
        }
    }

    @Test
    void listTablesFindsUserTables() throws SQLException {
        try (Connection c = mem(); Statement st = c.createStatement()) {
            st.execute("create table foo(id integer)");
            st.execute("create table bar(id integer)");
            List<String> tables = new SqliteDialect().listTables(c, "main");
            assertTrue(tables.contains("foo"));
            assertTrue(tables.contains("bar"));
        }
    }

    @Test
    void factoryResolvesByType() {
        assertTrue(Dialects.of("sqlite") instanceof SqliteDialect);
        assertTrue(Dialects.of("mysql") instanceof MySqlDialect);
    }
}
