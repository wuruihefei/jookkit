package com.jookkit.dialect;

import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.List;

public final class SqliteDialect implements Dialect {
    @Override
    public List<String> listDatabases(Connection c) {
        return List.of("main");
    }

    @Override
    public List<String> listTables(Connection c, String db) throws SQLException {
        List<String> out = new ArrayList<>();
        String sql = "select name from sqlite_master where type='table' "
                + "and name not like 'sqlite_%' order by name";
        try (Statement st = c.createStatement(); ResultSet rs = st.executeQuery(sql)) {
            while (rs.next()) out.add(rs.getString(1));
        }
        return out;
    }

    @Override
    public String quote(String identifier) {
        return "\"" + identifier.replace("\"", "\"\"") + "\"";
    }
}
