# 企业微信消息推送服务器

## 功能
- 收到config.czh中定义的自动回复，会回复指定内容。
- 收到以`/`开头的会被识别为命令，使用命令必须以`/`开头
- 只有管理员可使用命令

## 使用方法
- 需要一个企业微信
### config.czh
- CorpID                  我的企业/企业信息/企业ID
- CorpSecret              应用管理/xxx/Secret
- Token和EncodingAESKey   应用管理/xxx/功能/设置API接收/
- 以上xxx代表应用名称，没有就创建一个
- tags为自动回复
- admin为管理员，使用true启用
### 依赖
```
openssl 
curl
```
### 编译
```
gcc main.cpp -lstdc++ -lcrypto -lcurl -lpthread -ltinyxml2 -o wxserver
./wxserver -c config.czh
```

## 添加命令
- 使用`add_cmd(const std::string& tag, const Cmd_func& func)`添加命令,第一个参数是命令(不含`/`)，第二个参数是回调函数
- 回调函数接收一个const std::string&, 返回WXcmd_ret
- Cmd_func 即为 std::function<Cmd_ret(const std::string&)>
- Cmd_ret 即为 std::pair<const std::string, const std::string>，第一个string表发送类型(目前仅支持file,text)，第二个表内容。当发送类型为file时，内容为该文件路径
- 当服务器收到命令时，会向回调函数传递参数。`/`后的第一个空格分离命令和其参数,例`/file abc.txt`，命令为`file`,参数为`abc.txt`

## 注意事项
- 使用`file`命令和添加的命令发送文件时， 5B<文件大小<20MB

## 参考
[企业微信API](https://work.weixin.qq.com/api/doc)  
[C++ 写的UrlEncode和UrlDecode实例](https://www.jb51.net/article/201855.htm)  
[C++用libcurl通过HTTP以表单的方式Post数据到服务器](https://blog.csdn.net/shaoyiju/article/details/78238336)  
[用C++写一个简单的服务器(Linux)](https://blog.csdn.net/qq_29695701/article/details/83830108)  
[C/C++中的__FUNCTION__，__FILE__和__LINE__](https://www.cnblogs.com/yooyoo/p/4717917.html)  
[基于C++11的线程池(threadpool),简洁且可以带任意多的参数](https://www.cnblogs.com/lzpong/p/6397997.html)  
