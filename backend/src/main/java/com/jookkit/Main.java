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

        // 看门狗:前端通过 stdin 管道与本进程相连;前端一旦退出(正常关闭/被杀/崩溃),
        // 管道关闭、read 返回 -1,据此自我退出,避免后端被孤儿化遗留在后台。
        Thread watchdog = new Thread(() -> {
            try {
                while (System.in.read() != -1) { /* 持续读取,忽略内容 */ }
            } catch (Exception ignored) {
            }
            System.exit(0);  // 触发上面的 shutdown hook
        }, "parent-watchdog");
        watchdog.setDaemon(true);
        watchdog.start();

        Thread.currentThread().join(); // 阻塞主线程,保持服务运行
    }
}
