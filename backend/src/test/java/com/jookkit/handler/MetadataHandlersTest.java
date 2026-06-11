package com.jookkit.handler;

import com.google.gson.JsonObject;
import com.jookkit.conn.ConnectionRegistry;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class MetadataHandlersTest {
    private ConnectionRegistry reg = new ConnectionRegistry();

    private JsonObject openSqlite(String id) {
        JsonObject o = new JsonObject();
        o.addProperty("connId", id);
        o.addProperty("type", "sqlite");
        o.addProperty("file", ":memory:");
        return o;
    }

    @Test
    void openListTablesAndDescribe() {
        ConnectionHandlers.Open open = new ConnectionHandlers.Open(reg);
        open.handle(openSqlite("c1"));

        ExecSqlHandler exec = new ExecSqlHandler(reg);
        JsonObject ddl = new JsonObject();
        ddl.addProperty("connId", "c1");
        ddl.addProperty("sql", "create table person(id integer primary key, name text not null)");
        exec.handle(ddl);

        // LIST_TABLES
        JsonObject lt = new JsonObject();
        lt.addProperty("connId", "c1");
        lt.addProperty("db", "main");
        JsonObject tables = new MetadataHandlers.ListTables(reg).handle(lt);
        assertTrue(tables.getAsJsonArray("tables").toString().contains("person"));

        // DESCRIBE_TABLE
        JsonObject dt = new JsonObject();
        dt.addProperty("connId", "c1");
        dt.addProperty("db", "main");
        dt.addProperty("table", "person");
        JsonObject desc = new MetadataHandlers.DescribeTable(reg).handle(dt);
        assertEquals(2, desc.getAsJsonArray("columns").size());
        assertEquals("id", desc.getAsJsonArray("columns").get(0)
                .getAsJsonObject().get("name").getAsString());
    }

    @Test
    void listDatabasesReturnsMainForSqlite() {
        ConnectionHandlers.Open open = new ConnectionHandlers.Open(reg);
        open.handle(openSqlite("c2"));
        JsonObject q = new JsonObject();
        q.addProperty("connId", "c2");
        JsonObject dbs = new MetadataHandlers.ListDatabases(reg).handle(q);
        assertEquals("main", dbs.getAsJsonArray("databases").get(0).getAsString());
    }
}
