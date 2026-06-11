package com.jookkit.server;

import com.google.gson.JsonElement;
import com.google.gson.JsonNull;
import com.google.gson.JsonObject;

public final class Protocol {
    private Protocol() {}

    public static JsonObject ok(int funcId, JsonElement data) {
        JsonObject o = new JsonObject();
        o.addProperty("funcId", funcId);
        o.addProperty("ok", true);
        o.add("data", data == null ? JsonNull.INSTANCE : data);
        return o;
    }

    public static JsonObject error(int funcId, String code, String message, String sqlState) {
        JsonObject o = new JsonObject();
        o.addProperty("funcId", funcId);
        o.addProperty("ok", false);
        JsonObject err = new JsonObject();
        err.addProperty("code", code);
        err.addProperty("message", message);
        if (sqlState != null) err.addProperty("sqlState", sqlState);
        o.add("error", err);
        return o;
    }
}
