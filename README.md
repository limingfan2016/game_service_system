# game_service_system
base lib, connect lib, db operator lib, develop frame, and game engine, game frame game service system!
从0开始开发 基础库（配置文件读写、日志、多线程、多进程、锁、对象引用计数、内存池、免锁消息队列、免锁数据缓冲区、进程信号、共享内存、定时器等等基础功能组件），网络库（socket、TCP、UDP、epoll机制、连接自动收发消息等等），数据库操作库（mysql，redis、memcache API 封装可直接调用），开发框架库（消息调度处理、自动连接管理、服务开发、游戏框架、服务间消息收发、消息通信等等），消息中间件服务（不同网络节点间自动传递收发消息）等多个功能组件、服务，最后完成一套完整的服务器引擎，基于该框架引擎可开发任意的网络服务。  主体架构：N网关+N服务+N数据库代理+内存DB（Redis、MemCache）+Mysql数据库，基于该架构可建立集群，稳定高效的处理大规模、高并发消息。
