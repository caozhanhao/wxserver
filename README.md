<h2 align="center">
wxserver
</h2> 

<p align="center">
<strong>C++ Header-only 企业微信消息推送服务器</strong>
</p>

### 示例

- 下面是用wxserver接入的ChatGPT
- 详细[搭建过程](docs/zhihu.md)

#### server

![server](examples/pic/wxserver-server.png)

#### weixin

![weixin](examples/pic/wxserver-weixin.jpg)

### 使用方法

- 下面是一个消息回复示例

```c++
ws::Server server;
server.load_config("config.czh");
server.add_msg_handle(
    [&server](const ws::Request &req, ws::Message &res)
    {
          res.set_content(ws::MsgType::text, "hello, world");
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

#### 直接发送消息

```c++
server.send_message({ws::MsgType::text, "hello", "caozhanhao"});
server.send_message({ws::MsgType::file, "LICENSE", "caozhanhao"});
server.send_message({ws::MsgType::image, "example.jpg", "caozhanhao"});
server.send_message({ws::MsgType::markdown, "`hello world`", "caozhanhao"});
// ... 同set_content
```

#### 初始化

##### 直接初始化

```c++
Server(int port_, int agent_id, const std::string& token, const std::string encoding_aes_key,
           const std::string corp_id, const std::string corp_secret,
           bool enable_console_logger, const std::string& logging_path = "")
```

##### 从文件初始化

```c++
ws::Server server;
server.load_config("config.czh");
// or
// auto config = ws::parse_config("config.czh");
// server.load_config(config);
```

###### config.czh

| server               | info            |
|----------------------|-----------------|
| port                 | 运行端口         |
| enable_console_logger      | 日志控制台输出, false不输出 |
| logging_path | 日志文件, null不输出到文件 |

| weixin               | info                   |
|----------------------|------------------------|
| AgentId              | 位于我的企业/xxx/AgentId     |
| CorpID               | 位于我的企业/企业信息/企业ID       |
| CorpSecret           | 位于应用管理/xxx/Secret      |
| Token和EncodingAESKey | 位于应用管理/xxx/功能/设置API接收/ |

注：以下为[示例](examples/src/bot.hpp)中需要的配置文件，不是wxserver必须的

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
### 编译

#### linux

```shell
g++ examples/src/main.cpp -I examples/src -I include -I thirdparty -I thirdparty/json/include -I thirdparty/libczh/include -lssl -lcrypto -lpthread -O2 -std=c++2a -o wxserver-linux
```

#### windows

- 注意将OpenSSL的目录替换为你自己的。
```shell
g++ examples/src/main.cpp -I examples/src -I "C:\Program Files\OpenSSL-Win64\include" -I include -I thirdparty -I thirdparty/json/include -I thirdparty/libczh/include -L "C:\Program Files\OpenSSL-Win64\lib" -lssl -lcrypto -l ws2_32 -l crypt32 -lpthread -O2 -std=c++2a -o wxserver-windows.exe
```


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