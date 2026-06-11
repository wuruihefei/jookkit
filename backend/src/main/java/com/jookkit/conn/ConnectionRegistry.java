package com.jookkit.conn;

import com.jookkit.server.JookException;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.util.Map;
import java.util.Properties;
import java.util.concurrent.ConcurrentHashMap;

public final class ConnectionRegistry {
    private final Map<String, Connection> conns = new ConcurrentHashMap<>();

    public void open(ConnectionConfig cfg) {
        try {
            Properties props = new Properties();
            if (cfg.user != null) props.setProperty("user", cfg.user);
            if (cfg.password != null) props.setProperty("password", cfg.password);
            Connection c = DriverManager.getConnection(cfg.jdbcUrl(), props);
            conns.put(cfg.connId, c);
        } catch (SQLException e) {
            throw new JookException("CONN_ERROR", e.getMessage(), e.getSQLState());
        }
    }

    public Connection get(String connId) {
        Connection c = conns.get(connId);
        if (c == null) throw new JookException("NO_CONN", "no such connection: " + connId);
        return c;
    }

    public void close(String connId) {
        Connection c = conns.remove(connId);
        if (c != null) {
            try { c.close(); } catch (SQLException ignored) {}
        }
    }

    public void closeAll() {
        for (String id : conns.keySet()) close(id);
    }
}
