package com.jookkit.codec;

import com.google.gson.JsonObject;
import org.junit.jupiter.api.Test;
import java.sql.*;
import static org.junit.jupiter.api.Assertions.*;

class ResultSetCodecTest {
    private Connection mem() throws SQLException {
        return DriverManager.getConnection("jdbc:sqlite::memory:");
    }

    @Test
    void encodesColumnsRowsAndNull() throws SQLException {
        try (Connection c = mem(); Statement st = c.createStatement()) {
            st.execute("create table t(id integer, name text)");
            st.execute("insert into t values (1, 'a'), (2, null)");
            try (ResultSet rs = st.executeQuery("select id, name from t order by id")) {
                JsonObject out = ResultSetCodec.encode(rs);
                assertEquals(2, out.getAsJsonArray("columns").size());
                assertEquals("id", out.getAsJsonArray("columns").get(0)
                        .getAsJsonObject().get("name").getAsString());
                assertEquals(2, out.getAsJsonArray("rows").size());
                // 第二行 name 为 null
                assertTrue(out.getAsJsonArray("rows").get(1).getAsJsonArray()
                        .get(1).isJsonNull());
                assertEquals("a", out.getAsJsonArray("rows").get(0).getAsJsonArray()
                        .get(1).getAsString());
            }
        }
    }
}
