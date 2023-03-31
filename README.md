<h2 align="center">
wxserver
</h2> 

<p align="center">
<strong>C++ Header-only 企业微信消息推送服务器</strong>
</p>

### server

![server](examples/wxserver-server.png)

### weixin

![weixin](examples/wxserver-weixin.jpg)

### 使用方法

- 下面是一个消息回复示例

```c++
ws::Server server;
server.load_config("config.czh");
server.add_msg_handle(
    [&server](const ws::Request &req, ws::Response &res)
    {
      if (req.content == "license")
        res.set_file("LICENSE");
    });
server.run();
```

#### add_msg_handle

添加当接受的消息时的回调函数

```c++
[&server](const ws::Request &req, ws::Response &res){}
```

##### ws::Resquest

- `user_id` 消息发送者的id
- `content` 消息内容。

##### ws::Response

- `set_user(user_id)` 回复的用户，默认为消息发送者
- `set_text(str)`  回复字符串
- `set_file(path)` 回复文件
  `ws::Response`是在消息请求的响应包上带的消息，也可以异步发送消息, 如下

```c++
server.add_msg_handle(
    [&server](const ws::Request &req, ws::Response &res)
    {
      std::thread(
          [req, &server]()
          {
            //do something
            server.send_text(ret, req.user_id);
          }).detach();
    });
```

### config.czh

| server               | info |
|----------------------|------|
| port                 | 运行端口 |
| logging_path | 日志文件 |

| weixin               | info                   |
|----------------------|------------------------|
| CorpID               | 位于我的企业/企业信息/企业ID       |
| CorpSecret           | 位于应用管理/xxx/Secret      |
| Token和EncodingAESKey | 位于应用管理/xxx/功能/设置API接收/ |

| hugging_face | info                                    |
|--------------|-----------------------------------------|
| token        | [示例](examples/main.cpp)中接入了Hugging Face |

- 以上xxx代表应用名称，没有就创建一个

### 编译

```
mkdir build && cd build 
cmake .. && make
./wxserver 
```

### 注意事项

- 发送文件时，文件须小于20 MB

### 依赖

- openssl
- [nlohmann/json](https://github.com/nlohmann/json)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [libczh](https://github.com/caozhanhao/libczh)
- 需C++ 20
