package com.jookkit.dialect;

public final class Dialects {
    private Dialects() {}

    public static Dialect of(String type) {
        switch (type) {
            case "sqlite": return new SqliteDialect();
            case "mysql":  return new MySqlDialect();
            default: throw new IllegalArgumentException("unknown type: " + type);
        }
    }
}
