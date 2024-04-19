#include <event.h>
#include <event2/listener.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "log.h"

#include "pub.c"

const short PORT = 9096;
const char *_DIR_PREFIX_FILE_ = "res/http_res/dir_header.html";
const char *_DIR_TAIL_FILE_ = "res/http_res/dir_tail.html";

void initLog()
{
    if (msgInit("HTTPserver") != 0)
    {
        fprintf(stderr, "faile to init log\n");
        exit(1);
    }
}

void closeLog()
{
    msgLogClose();
}

/**
 *@redo 怎么解决中文乱码问题
 */
int copy_file(struct bufferevent *bev, const char *strFile)
{
    int fd = open(strFile, O_RDONLY);
    if (fd < 0)
    {
        perror("open erro");
        return -1;
    }
    char buf[1024] = {0};
    int ret;
    while ((ret = read(fd, buf, sizeof(buf))) > 0)
    {
        bufferevent_write(bev, buf, ret);
    }
    close(fd);
    return 0;
}

/**
 * @brief 发送目录、实际发送一个HTML文件给客户端、目录的内容作为列表的方式显示
 */
int send_dir(struct bufferevent *bev, const char *strpath)
{
    // 需要拼出一个HTML页面给客户端
    copy_file(bev, _DIR_PREFIX_FILE_);
    DIR *dir = opendir(strpath);
    if (dir == NULL)
    {
        perror("opendir err");
        return -1;
    }
    char bufline[1024] = {0};
    struct dirent *dent = NULL;
    while (dent = readdir(dir))
    {
        int len = 0;
        struct stat fileStat;
        stat(dent->d_name, &fileStat);
        memset(bufline, 0, sizeof(bufline));
        if (dent->d_type == DT_DIR)
        {
            len = sprintf(bufline, "<li><a href='%s/'>%32s</a>   %8ld</li>", dent->d_name, dent->d_name, fileStat.st_size);
        }
        else if (dent->d_type == DT_REG)
        {
            len = sprintf(bufline, "<li><a href='%s'>%32s</a>   %8ld</li>", dent->d_name, dent->d_name, fileStat.st_size);
        }
        bufferevent_write(bev, bufline, len);
    }
    closedir(dir);
    copy_file(bev, _DIR_TAIL_FILE_);
    return 0;
}

/**
 * 发送响应头
 */
int copy_header(struct bufferevent *bev, int statcode, char *msg, char *filetype, long filesize)
{
    char buf[4096] = "";
    sprintf(buf, "HTTP/1.1 %d %s\r\n", statcode, msg);
    sprintf(buf, "%sContent-Type:%s\r\n", buf, filetype);
    if (filesize >= 0)
    {
        sprintf(buf, "%sContent-Length:%ld\r\n", buf, filesize);
    }
    strcat(buf, "\r\n");
    bufferevent_write(bev, buf, strlen(buf));
    return 0;
}

int http_request(struct bufferevent *bev, char *path)
{
    strdecode(path, path); // 将中文问题转码成utf-8格式的字符串
    char *strpath = path;
    if (strcmp(strpath, "/") == 0 || strcmp(strpath, "/.") == 0)
    {
        strpath = "./";
    }
    else
    {
        // 跳过根目录
        strpath = path + 1;
    }

    struct stat fileStat;

    if (stat(strpath, &fileStat) < 0)
    {
        copy_header(bev, 404, "NOT FOUND", get_mime_type("error.html"), -1);
        copy_file(bev, "error.html");
        return -1;
    }
    if (S_ISDIR(fileStat.st_mode))
    {
        copy_header(bev, 200, "OK", get_mime_type("*.html"), fileStat.st_size);
        send_dir(bev, strpath);
    }
    if (S_ISREG(fileStat.st_mode))
    {
        copy_header(bev, 200, "OK", get_mime_type(strpath), fileStat.st_size);
        copy_file(bev, strpath);
    }
    return 0;
}
void bufevent_cd(struct bufferevent *bufEv, short event, void *arg)
{
    if (event & BEV_EVENT_EOF) // 客户端关闭
    {
        printf("client closed\n");
        msglog(MSG_INFO, "")
            bufferevent_free(bufEv);
    }
    else if (event & BEV_EVENT_ERROR)
    {
        printf("err to client closed\n");
        bufferevent_free(bufEv);
    }
    else if (event & BEV_EVENT_CONNECTED)
    {
        printf("client connet ok\n");
    }
}
/**
 * @brief bufferevent的读回调函数
 *
 */
void read_cb(struct bufferevent *bev, void *ctx)
{
    char buf[256] = "";
    char method[10], path[256], protocol[10];

    // 先读出请求头
    int ret = bufferevent_read(bev, buf, sizeof(buf));
    if (ret > 0)
    {
        // 解析请求头
        sscanf(buf, "%[^ ] %[^ ] %[^ \r\n]", method, path, protocol);
        if (strcasecmp(method, "get") == 0)
        {
            char bufline[256] = "";
            write(STDOUT_FILENO, buf, ret);
            // 读取请求体、忽略
            while ((ret = bufferevent_read(bev, bufline, sizeof *bufline)) > 0)
            {
                // 打印请求体
                write(STDOUT_FILENO, bufline, ret);
            }
            http_request(bev, path);
        }
    }
}

void listen_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int socklen, void *arg)
{
    printf("new conneciton...\n");
    struct event_base *base = (struct event_base *)arg;
    struct bufferevent *bufEv = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bufEv, read_cb, NULL, bufevent_cd, base); // 设置bufferevnet的三种回调函数
    bufferevent_enable(bufEv, EV_READ | EV_WRITE);
}

int main()
{
    initLog();
    // 修改当前目录为当前工作目录
    char workdir[256] = {0};
    strcpy(workdir, getenv("PWD"));
    chdir(workdir);

    // 构建bufevent的红黑树
    struct event_base *base = event_base_new();
    struct sockaddr_in servAddr;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(PORT);
    servAddr.sin_family = AF_INET;

    // 创建监听器--监听客户端的连接请求
    struct evconnlistener *listener = evconnlistener_new_bind(base, listen_cb, base, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, (struct sockaddr *)&servAddr, sizeof(servAddr));

    event_base_dispatch(base); // 开启事件循环
    event_base_free(base);
    evconnlistener_free(listener);

    return 0;
}