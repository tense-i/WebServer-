#include <stdio.h>
#include <stdlib.h>

#include "log.h"

#include "log.c"

int main()
{
    // 初始化日志系统
    if (msgInit("TestApp") != 0)
    {
        fprintf(stderr, "Failed to initialize log system\n");
        return 1;
    }

    // 记录普通信息日志
    msglog(MSG_INFO, "This is a normal log message");

    // 记录警告信息日志
    msglog(MSG_WARN, "This is a warning log message");

    // 关闭日志系统
    msgLogClose();

    return 0;
}
