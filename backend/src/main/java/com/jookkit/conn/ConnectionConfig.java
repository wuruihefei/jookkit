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
    public String params;     // 可选:附加 JDBC 参数,如 "useSSL=false&serverTimezone=GMT%2B8"

    public String jdbcUrl() {
        switch (type) {
            case "sqlite":
                return "jdbc:sqlite:" + file;
            case "mysql": {
                String url = "jdbc:mysql://" + host + ":" + port + "/"
                        + (database == null ? "" : database);
                if (params != null && !params.isEmpty()) url += "?" + params;
                return url;
            }
            default:
                throw new IllegalArgumentException("unknown type: " + type);
        }
    }
}
