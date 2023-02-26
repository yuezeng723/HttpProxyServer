
unordered_map<string, vector<char>> cache // request第一行 ：target server的response全部
unordered_map<string, targetServerResponse> validationMap 
class TargetServerResponse {
    time when I receive
    time max-age
    
    date
    lastmodify && expiretime
    先后顺序
    lastmodify date算默认max-age时间 /10 
}
把client request全部读进buffer里，判断method，hostname， port
判断method：
1. connect
    1.1 和target server创建socket
    1.2 给client 发送 200OK
    1.3 一边读一边转发target server给client的response，直到读到0 byte.
    1.4 关闭proxy 和target server之间的socket
2. get
    2.1 在 cache 里去查找是否存了get request对应的response
        2.1.1 如果有，检查validationMap里面time有没有过期
            2.1.1.1 如果过期了
                    从server里读新的response，更新cache和validationMap
            2.1.1.2 如果没过期，直接从cache里发给client
        2.1.2 如果没有
                    从server里读新的response，更新cache和validationMap
3. post
    直接转发

yichen:
1. parse host name, parse port : string getter
2. time validation;
3. deamon 