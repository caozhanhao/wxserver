# 企业微信回复消息服务器

## 功能
2021.3.7

运行时可以通过 微信/企业微信 向应用发送消息，程序收到消息会提示。
如果收到config.yaml中定义的自动回复，会回复指定内容。
如果收到config.yaml中定义的管理员发送的 pic/file ，会发送指定图片或文件。
例(向企业微信应用发送):
```
image /home/example_user/example.jpg
file /home/example_user/example.cpp
```
相应的命令在config.yaml的command中，目前只测试了file(文件)和image(图片)

## 依赖
使用时需要config.yaml(wxsend.cpp和server.cpp都使用)，把wxsend和server放在同一目录  

库：
	tinyxml2: tinyxml2-2.1.0
	openssl: openssl-1.0.1h
	yaml-cpp : yaml-cpp 0.6.3-2

## 已知不足
如果把wxsend写成成员函数，libcurl会segmentation fault，暂不知如何解决，所以wxsend单独编译  
因此，也可以在命令行中单独调用
```	
./wxsend [type] [item] [UserID]
```
例：
```	
./wxsend text example @all
./wxsend image example.jpg @all
./wxsend file example.cpp @all
```
## 编译方法

### wxsend 
```
clang++ wxsend.cpp /usr/lib/libyaml-cpp.so.0.6 -lcurl -o wxsend
```
### Others (vs2019 or clang++)

#### 1. vs2019（连接Arch Linux）
添加其他文件到项目，不包括wxsend.cpp  
项目属性-链接器-输入-附加依赖项：(要包括路径)  
```	
libcrypto.a
libtinyxml2.so.7
libyaml-cpp.so.0.6
libcurl.so
```
#### 2. clang++（Arch Linux 示例）
```
clang++ server.cpp WXBizMsgCrypt.cpp /usr/lib/libtinyxml2.so.7 /usr/local/openssl/lib/libcrypto.a /usr/lib/libyaml-cpp.so.0.6 -o server

```
## 参考
[企业微信API](https://work.weixin.qq.com/api/doc)
[C++ 写的UrlEncode和UrlDecode实例](https://www.jb51.net/article/201855.htm)
[C++用libcurl通过HTTP以表单的方式Post数据到服务器](https://blog.csdn.net/shaoyiju/article/details/78238336)
[c++ 开发中利用yaml-cpp读写yaml配置文件](https://blog.csdn.net/briblue/article/details/89515470)
[yaml-cpp Easiest way to iterate through a map with undefined value](https://stackoverflow.com/questions/12374691/yaml-cpp-easiest-way-to-iterate-through-a-map-with-undefined-values)
[用C++写一个简单的服务器(Linux)](https://blog.csdn.net/qq_29695701/article/details/83830108)
......

