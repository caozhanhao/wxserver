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
  ws::Server server("config.czh");
server.add_msg_handle(
[&server](const ws::Request &req, ws::Response &res)
{
if (req.content == "license")
{
res.set_file("LICENSE");
}
});
server.run();
```

- 通过上述方法也可接入一个AI, 在[bot_example.hpp](src/bot_example.hpp)用Hugging Face接入了blenderbot。
- 使用该示例时需要在运行目录有一个`hugging_face.czh`, 内容为`token = "Your access token"`

### config.czh

| 配置                   | 相关信息                   |
|----------------------|------------------------|
| Port                 | 即wxserver运行端口          |
| CorpID               | 位于我的企业/企业信息/企业ID       |
| CorpSecret           | 位于应用管理/xxx/Secret      |
| Token和EncodingAESKey | 位于应用管理/xxx/功能/设置API接收/ |

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
