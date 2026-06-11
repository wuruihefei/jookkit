package com.jookkit;

import com.jookkit.conn.ConnectionRegistry;
import com.jookkit.server.Dispatcher;
import com.jookkit.server.JsonHttpServer;

public final class Main {
    public static void main(String[] args) throws Exception {
        int port = 0; // 0 = 由系统分配随机端口
        for (int i = 0; i < args.length - 1; i++) {
            if ("--port".equals(args[i])) port = Integer.parseInt(args[i + 1]);
        }
        ConnectionRegistry registry = new ConnectionRegistry();
        JsonHttpServer server = new JsonHttpServer(port, new Dispatcher(registry));
        server.start();
        // 关键:把实际端口打到 stdout,供前端 QProcess 读取握手
        System.out.println("JOOKKIT_PORT=" + server.port());
        System.out.flush();
        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            registry.closeAll();
            server.stop();
        }));
        Thread.currentThread().join(); // 阻塞主线程,保持服务运行
    }
}
