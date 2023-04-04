<h2 align="center">
wxserver
</h2> 

<p align="center">
<strong>C++ Header-only 企业微信消息推送服务器</strong>
</p>

下面是用wxserver接入的ChatGPT

### server

![server](examples/pic/wxserver-server.png)

### weixin

![weixin](examples/pic/wxserver-weixin.jpg)

### 使用方法

- 下面是一个消息回复示例

```c++
ws::Server server;
server.load_config("config.czh");
server.add_msg_handle(
    [&server](const ws::Request &req, ws::Message &res)
    {
      if (req.content == "license")
        res.set_file("LICENSE");
    });
server.run();
```

#### add_msg_handle

添加当接受的消息时的回调函数

```c++
[&server](const ws::Request &req, ws::Message &res){}
```

##### ws::Resquest

- `user_id` 消息发送者的id
- `content` 消息内容。

##### ws::Message

###### `set_user(user_id)`
指定回复的用户，默认为消息发送者
###### `set_content(MsgType, data)`
指定消息回复内容

| MsgType              | info                                                                                                                         |
|----------------------|------------------------------------------------------------------------------------------------------------------------------|
| text              | 文本                                                                                                                           |
| markdown               | [markdown子集](https://developer.work.weixin.qq.com/document/path/90236#%E6%94%AF%E6%8C%81%E7%9A%84markdown%E8%AF%AD%E6%B3%95) |
| image           | 图片(path)                                                                                                                     |
| file | 文件(path)                                                                                                                     |

```c++
server.add_msg_handle(
    [&server](const ws::Request &req, ws::Message &res)
    {
      std::thread(
          [req, &server]()
          {
            //do something
            res.set_content(ws::MsgType::text, ret);
          }).detach();
    });
```

##### 直接发送消息

```c++
server.send_message({ws::MsgType::text, "hello", "caozhanhao"});
server.send_message({ws::MsgType::file, "LICENSE", "caozhanhao"});
server.send_message({ws::MsgType::image, "example.jpg", "caozhanhao"});
server.send_message({ws::MsgType::markdown, "`hello world`", "caozhanhao"});
// ... 同set_content
```

### config.czh

- 以下是必要的配置

| server               | info |
|----------------------|------|
| port                 | 运行端口 |
| logging_path | 日志文件 |

| weixin               | info                   |
|----------------------|------------------------|
| AgentId              | 位于我的企业/xxx/AgentId     |
| CorpID               | 位于我的企业/企业信息/企业ID       |
| CorpSecret           | 位于应用管理/xxx/Secret      |
| Token和EncodingAESKey | 位于应用管理/xxx/功能/设置API接收/ |

- 以下为示例中需要的配置，非必须
  [示例](examples/src/main.cpp)中接入了HuggingFace和OpenAI ChatGPT

| hugging_face | info                                          |
|--------------|-----------------------------------------------|
| model        | 需Conversational，如ingen51/DialoGPT-medium-GPT4 |
| token        | HuggingFace token                             |

| openai     | info           |
|------------|----------------|
| model      | 如gpt-3.5-turbo |
| token      | OpenAI token   |
| proxy      | Http代理地址       |
| proxy_port | Http代理端口       |

- 以上xxx代表应用名称，没有就创建一个

### 编译

- linux

```shell
g++ examples/src/main.cpp -I examples/src -I include -I thirdparty -I thirdparty/json/include -I thirdparty/libczh/include -lssl -lcrypto -lpthread -O2 -std=c++2a -o wxserver-linux
```

- windows

```shell
g++ examples/src/main.cpp -I examples/src -I "C:\Program Files\OpenSSL-Win64\include" -I include -I thirdparty -I thirdparty/json/include -I thirdparty/libczh/include -L "C:\Program Files\OpenSSL-Win64\lib" -lssl -lcrypto -l ws2_32 -l crypt32 -lpthread -O2 -std=c++2a -o wxserver-windows.exe
```

注意将OpenSSL的目录替换为你自己的。

### 注意事项

- 发送文件时，文件须小于20 MB

### 依赖

- openssl
- [nlohmann/json](https://github.com/nlohmann/json)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [libczh](https://github.com/caozhanhao/libczh)
- 需C++ 20

### 贡献

- 欢迎PR

### 相关链接

- [搭建微信机器人（可接入ChatGPT等）](https://zhuanlan.zhihu.com/p/618651568)