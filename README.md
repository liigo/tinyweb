tinyweb
=======

Tinyweb, a tiny web server based on libuv, by liigo, 2013/06.

Tinyweb 是我用libuv开发的一个最精简的Web server服务器。分为三个版本，都是从真实项目中剥离出来的，从 v1 到 v2 到 v3，就是一个Web server从雏形到基本功能逐渐丰富的过程。

tinyweb v1，是最基础的libuv的hello world，可用来学习libuv的入门用法；tinyweb v2，在v1的基础上，添加了解析HTTP GET请求（Request）提取`path_info`和`query_string`并发送回复（Respone）的功能；tinyweb v3，在v2的基础上，又添加了对静态文件的支持。

真正在项目中有实用价值的，我认为应该是从tinyweb v2开始引入的对`path_info`的响应处理：在项目中嵌入tinyweb服务器，响应特定`path_info`，或输出内部运行状态，或触发某个动作，如此一来，用户（或开发者自己）通过Web浏览器即可轻松完成与项目程序的有效沟通，至少免除了进程通讯之类的东西吧，通过特殊的`path_info`（比如`http://localhost/hibos`）给自己的程序留一个小小的后门也是轻而易举。

CSDN博客文章链接：

 - [基于libuv的最精简Web服务器：tinyweb v1 v2 v3（C语言源码）](http://blog.csdn.net/liigo/article/details/38424417)
 - [tinyweb: C语言 + libuv 开发的最精简的WebServer（附源码）](http://blog.csdn.net/liigo/article/details/9149415)
