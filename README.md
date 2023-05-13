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
Server(std::string host, int port_, int agent_id, const std::string& token, const std::string encoding_aes_key,
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

| server                | info              |
|-----------------------|-------------------|
| host                  | 运行地址              |
| port                  | 运行端口              |
| enable_console_logger | 日志控制台输出, false不输出 |
| logging_path          | 日志文件, null不输出到文件  |

| weixin               | info                   |
|----------------------|------------------------|
| AgentId              | 位于我的企业/xxx/AgentId     |
| CorpID               | 位于我的企业/企业信息/企业ID       |
| CorpSecret           | 位于应用管理/xxx/Secret      |
| Token和EncodingAESKey | 位于应用管理/xxx/功能/设置API接收/ |

注：以下为[示例](examples/src/bot.hpp)中需要的配置文件，不是wxserver必须的

| bot                | info              |
|-----------------------|-------------------|
| bot                  | "hugging_face"或 "chatgpt" |
| proxy      | Http代理地址，不需要则设为null       |
| proxy_port | Http代理端口，不需要则设为null       |

| hugging_face | info                                          |
|--------------|-----------------------------------------------|
| model        | 需Conversational，如ingen51/DialoGPT-medium-GPT4 |
| token        | HuggingFace token                             |

| openai     | info           |
|------------|----------------|
| model      | 如"gpt-3.5-turbo" |
| token      | OpenAI token   |

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

- [搭建微信机器人（可接入ChatGPT等）](https://czh.netlify.app/2023/04/01/%E6%90%AD%E5%BB%BA%E5%BE%AE%E4%BF%A1%E6%9C%BA%E5%99%A8%E4%BA%BA%EF%BC%88%E5%8F%AF%E6%8E%A5%E5%85%A5chatgpt%E7%AD%89%EF%BC%89/)