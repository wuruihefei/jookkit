package com.jookkit.handler;

import com.google.gson.JsonObject;

public interface Handler {
    /** 处理请求,返回响应的 data 部分。业务错误抛 JookException。 */
    JsonObject handle(JsonObject req);
}
