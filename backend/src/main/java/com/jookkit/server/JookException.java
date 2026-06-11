package com.jookkit.server;

public class JookException extends RuntimeException {
    public final String code;
    public final String sqlState;

    public JookException(String code, String message, String sqlState) {
        super(message);
        this.code = code;
        this.sqlState = sqlState;
    }

    public JookException(String code, String message) {
        this(code, message, null);
    }
}
