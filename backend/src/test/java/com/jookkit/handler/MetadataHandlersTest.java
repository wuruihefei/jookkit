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
    void getDdlReturnsCreateStatement() {
        new ConnectionHandlers.Open(reg).handle(openSqlite("c3"));
        JsonObject ddl = new JsonObject();
        ddl.addProperty("connId", "c3");
        ddl.addProperty("sql", "create table widget(id integer primary key, nm text)");
        new ExecSqlHandler(reg).handle(ddl);

        JsonObject q = new JsonObject();
        q.addProperty("connId", "c3");
        q.addProperty("table", "widget");
        JsonObject d = new MetadataHandlers.GetDdl(reg).handle(q);
        assertTrue(d.get("ddl").getAsString().toLowerCase().contains("create table"));
    }

    @Test
    void getSchemaReturnsColumnsPkAndIndexes() {
        new ConnectionHandlers.Open(reg).handle(openSqlite("c4"));
        ExecSqlHandler exec = new ExecSqlHandler(reg);
        JsonObject ddl = new JsonObject();
        ddl.addProperty("connId", "c4");
        ddl.addProperty("sql", "create table acct(id integer primary key, email text)");
        exec.handle(ddl);
        JsonObject idx = new JsonObject();
        idx.addProperty("connId", "c4");
        idx.addProperty("sql", "create unique index ux_email on acct(email)");
        exec.handle(idx);

        JsonObject q = new JsonObject();
        q.addProperty("connId", "c4");
        q.addProperty("table", "acct");
        JsonObject d = new MetadataHandlers.GetSchema(reg).handle(q);
        assertEquals(2, d.getAsJsonArray("columns").size());
        assertTrue(d.getAsJsonArray("primaryKeys").toString().contains("id"));
        assertTrue(d.getAsJsonArray("indexes").toString().contains("ux_email"));
    }

    @Test
    void listUsersUnsupportedForSqlite() {
        new ConnectionHandlers.Open(reg).handle(openSqlite("c5"));
        JsonObject q = new JsonObject();
        q.addProperty("connId", "c5");
        JsonObject d = new MetadataHandlers.ListUsers(reg).handle(q);
        assertFalse(d.get("supported").getAsBoolean());
        assertEquals(0, d.getAsJsonArray("users").size());
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
