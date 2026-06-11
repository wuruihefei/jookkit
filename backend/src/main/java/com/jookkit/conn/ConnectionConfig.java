package com.jookkit.conn;

public final class ConnectionConfig {
    public String connId;
    public String type;       // "mysql" | "sqlite"
    public String host;
    public int port;
    public String user;
    public String password;
    public String database;
    public String file;       // sqlite 用

    public String jdbcUrl() {
        switch (type) {
            case "sqlite":
                return "jdbc:sqlite:" + file;
            case "mysql":
                return "jdbc:mysql://" + host + ":" + port + "/"
                        + (database == null ? "" : database);
            default:
                throw new IllegalArgumentException("unknown type: " + type);
        }
    }
}
