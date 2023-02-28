<h1 align="center">
    DangerLog
</h1>

1. Based on searching, we found if the cache control header in response is empty, it represents no-cache flag. We did not add operations to deal with this corner case. In further development, empty cache control header should be detected and treated as no-cache.

2. We use LRU Cache to store data and set the capacity limit to 10. But sometimes the program still terminates because of exceeding storage limit. We may be able to solve it by decreasing cache capacity limit.

3. When we tested the program, the port number might be occupied. The probem was solved by adding freeAddress and close socket to our program.

4. Sometimes response received by proxy server and client are unprintable characters. The reason might be about encapsulation. We have not found the specific reason for that error, but in most cases GET method works well.