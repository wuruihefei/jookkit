package com.jookkit.codec;

import com.google.gson.JsonArray;
import com.google.gson.JsonNull;
import com.google.gson.JsonObject;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;

public final class ResultSetCodec {
    private ResultSetCodec() {}

    public static JsonObject encode(ResultSet rs) throws SQLException {
        ResultSetMetaData md = rs.getMetaData();
        int n = md.getColumnCount();

        JsonArray columns = new JsonArray();
        for (int i = 1; i <= n; i++) {
            JsonObject col = new JsonObject();
            col.addProperty("name", md.getColumnLabel(i));
            col.addProperty("type", md.getColumnTypeName(i));
            columns.add(col);
        }

        JsonArray rows = new JsonArray();
        while (rs.next()) {
            JsonArray row = new JsonArray();
            for (int i = 1; i <= n; i++) {
                Object v = rs.getObject(i);
                if (v == null) {
                    row.add(JsonNull.INSTANCE);
                } else if (v instanceof Number) {
                    row.add((Number) v);
                } else if (v instanceof Boolean) {
                    row.add((Boolean) v);
                } else {
                    row.add(rs.getString(i));
                }
            }
            rows.add(row);
        }

        JsonObject out = new JsonObject();
        out.add("columns", columns);
        out.add("rows", rows);
        return out;
    }
}
