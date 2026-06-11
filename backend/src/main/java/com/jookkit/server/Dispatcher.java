package com.jookkit.server;

import com.google.gson.JsonObject;
import com.jookkit.conn.ConnectionRegistry;
import com.jookkit.handler.*;
import java.util.HashMap;
import java.util.Map;

public final class Dispatcher {
    private final Map<Integer, Handler> handlers = new HashMap<>();
    private final ConnectionRegistry registry;

    public Dispatcher(ConnectionRegistry registry) {
        this.registry = registry;
        handlers.put(FuncId.TEST_CONNECTION, new ConnectionHandlers.Test());
        handlers.put(FuncId.OPEN_CONNECTION, new ConnectionHandlers.Open(registry));
        handlers.put(FuncId.CLOSE_CONNECTION, new ConnectionHandlers.Close(registry));
        handlers.put(FuncId.LIST_DATABASES, new MetadataHandlers.ListDatabases(registry));
        handlers.put(FuncId.LIST_TABLES, new MetadataHandlers.ListTables(registry));
        handlers.put(FuncId.DESCRIBE_TABLE, new MetadataHandlers.DescribeTable(registry));
        handlers.put(FuncId.GET_DDL, new MetadataHandlers.GetDdl(registry));
        handlers.put(FuncId.EXEC_SQL, new ExecSqlHandler(registry));
        handlers.put(FuncId.INSERT_ROW, new DataEditHandlers.Insert(registry));
        handlers.put(FuncId.UPDATE_ROW, new DataEditHandlers.Update(registry));
        handlers.put(FuncId.DELETE_ROW, new DataEditHandlers.Delete(registry));
    }

    public ConnectionRegistry registry() { return registry; }

    public JsonObject dispatch(JsonObject req) {
        int funcId = req.has("funcId") ? req.get("funcId").getAsInt() : -1;
        Handler h = handlers.get(funcId);
        if (h == null) {
            return Protocol.error(funcId, "UNKNOWN_FUNC", "unknown funcId: " + funcId, null);
        }
        try {
            return Protocol.ok(funcId, h.handle(req));
        } catch (JookException e) {
            return Protocol.error(funcId, e.code, e.getMessage(), e.sqlState);
        } catch (Exception e) {
            return Protocol.error(funcId, "INTERNAL", String.valueOf(e.getMessage()), null);
        }
    }
}
