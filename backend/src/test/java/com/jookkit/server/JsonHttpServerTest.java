package com.jookkit.server;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.jookkit.conn.ConnectionRegistry;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import static org.junit.jupiter.api.Assertions.*;

class JsonHttpServerTest {
    private JsonHttpServer server;
    private int port;

    @BeforeEach
    void start() throws Exception {
        server = new JsonHttpServer(0, new Dispatcher(new ConnectionRegistry()));
        server.start();
        port = server.port();
    }

    @AfterEach
    void stop() { server.stop(); }

    private JsonObject post(String body) throws Exception {
        HttpClient client = HttpClient.newHttpClient();
        HttpRequest req = HttpRequest.newBuilder()
                .uri(URI.create("http://127.0.0.1:" + port + "/rpc"))
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(body))
                .build();
        HttpResponse<String> resp = client.send(req, HttpResponse.BodyHandlers.ofString());
        assertEquals(200, resp.statusCode());
        return JsonParser.parseString(resp.body()).getAsJsonObject();
    }

    @Test
    void roundTripOpenAndExec() throws Exception {
        JsonObject r1 = post("{\"funcId\":1002,\"connId\":\"c1\",\"type\":\"sqlite\",\"file\":\":memory:\"}");
        assertTrue(r1.get("ok").getAsBoolean());

        JsonObject r2 = post("{\"funcId\":3001,\"connId\":\"c1\",\"sql\":\"select 7 as n\"}");
        assertTrue(r2.get("ok").getAsBoolean());
        assertEquals(7, r2.getAsJsonObject("data")
                .getAsJsonArray("rows").get(0).getAsJsonArray().get(0).getAsInt());
    }

    @Test
    void portIsNonZeroAfterStart() {
        assertTrue(port > 0);
    }
}
