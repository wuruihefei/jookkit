package com.jookkit.dialect;

import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.List;

public final class MySqlDialect implements Dialect {
    @Override
    public List<String> listDatabases(Connection c) throws SQLException {
        List<String> out = new ArrayList<>();
        try (Statement st = c.createStatement();
             ResultSet rs = st.executeQuery("SHOW DATABASES")) {
            while (rs.next()) out.add(rs.getString(1));
        }
        return out;
    }

    @Override
    public List<String> listTables(Connection c, String db) throws SQLException {
        List<String> out = new ArrayList<>();
        String sql = "SELECT table_name FROM information_schema.tables "
                + "WHERE table_schema = ? ORDER BY table_name";
        try (var ps = c.prepareStatement(sql)) {
            ps.setString(1, db);
            try (ResultSet rs = ps.executeQuery()) {
                while (rs.next()) out.add(rs.getString(1));
            }
        }
        return out;
    }

    @Override
    public String quote(String identifier) {
        return "`" + identifier.replace("`", "``") + "`";
    }
}
