package com.jookkit.server;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.nio.charset.StandardCharsets;

public final class JsonHttpServer {
    private final HttpServer http;
    private final Dispatcher dispatcher;

    public JsonHttpServer(int port, Dispatcher dispatcher) throws IOException {
        this.dispatcher = dispatcher;
        this.http = HttpServer.create(new InetSocketAddress("127.0.0.1", port), 0);
        this.http.createContext("/rpc", this::handle);
        this.http.setExecutor(null); // 默认串行 executor,足够自用
    }

    public void start() { http.start(); }

    public void stop() { http.stop(0); }

    public int port() { return http.getAddress().getPort(); }

    private void handle(HttpExchange ex) throws IOException {
        try {
            if (!"POST".equalsIgnoreCase(ex.getRequestMethod())) {
                writeJson(ex, 405, Protocol.error(-1, "METHOD", "POST only", null));
                return;
            }
            String body = new String(ex.getRequestBody().readAllBytes(), StandardCharsets.UTF_8);
            JsonObject req = JsonParser.parseString(body).getAsJsonObject();
            JsonObject resp = dispatcher.dispatch(req);
            writeJson(ex, 200, resp);
        } catch (Exception e) {
            writeJson(ex, 200, Protocol.error(-1, "BAD_REQUEST", String.valueOf(e.getMessage()), null));
        }
    }

    private void writeJson(HttpExchange ex, int status, JsonObject obj) throws IOException {
        byte[] bytes = obj.toString().getBytes(StandardCharsets.UTF_8);
        ex.getResponseHeaders().add("Content-Type", "application/json; charset=utf-8");
        ex.sendResponseHeaders(status, bytes.length);
        try (OutputStream os = ex.getResponseBody()) {
            os.write(bytes);
        }
    }
}
