
#include "../warpFun/wrap.h"
#include "pub.c"
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 9095
#define maxn 1024

enum EPOLLWEB
{
    NOT_CLOSE_EV,
    CLOSE_EV
};

void send_header(int cfd, int code, const char *info, const char *filetype, int length)
{
    // 发送状态行 404 client-err
    char buf[1024] = "";
    int len = sprintf(buf, "HTTP/1.1 %d %s\r\n", code, info);
    send(cfd, buf, len, 0);

    // 发送消息头 content-type:text/html; charset=utf-8\r\n
    len = sprintf(buf, "content-Type:%s\r\n", filetype);
    send(cfd, buf, len, 0);

    // 发送消息长度 -
    if (length > 0)
    {
        len = sprintf(buf, "Content-Length:%d\r\n", length);
        send(cfd, buf, len, 0);
    }

    // 发送空行
    send(cfd, "\r\n", 2, 0);
}

/**
 * @brief 发送指定文件
 */
void send_file(int cfd, const char *path, struct epoll_event *ev, int epfd, enum EPOLLWEB flg)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("open fail");
        return;
    }
    char buf[1024] = "";
    int len = 0;
    while (1)
    {
        len = read(fd, buf, sizeof(buf));
        if (len > 0)
        {
            int n = 0;
            n = send(cfd, buf, len, 0);
            printf("len=%d \n", n);
        }
        else if (len == 0)
        {
            break;
        }
        else
        {
            perror(":");
            break;
        }
    }
    close(fd);

    if (flg == CLOSE_EV)
    { // 关闭cfd、下树
        close(cfd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, ev->data.fd, ev);
    }
}

void ScanDir()
{
}
void read_client_request(int epfd, struct epoll_event *ev)
{
    char buf[1024];
    char tempBuf[1024];
    // 读取请求数据--请求行、请求体扔掉不管
    int readn = Readline(ev->data.fd, buf, sizeof(buf));
    if (readn <= 0)
    {
        printf("close or err \n");
        epoll_ctl(epfd, EPOLL_CTL_DEL, ev->data.fd, ev);
        close(ev->data.fd);
        return;
    }
    printf("read getheader: %s", buf);
    int ret = 0;
    while ((ret = Readline(ev->data.fd, tempBuf, sizeof(tempBuf))) > 0)
    {
        ;
    }
    printf("read ok \n");

    // 解析请求行 get /a.txt HTTP/1.1
    char method[10];
    char path[100];
    char protocol[20];

    // 使用 sscanf 提取请求中的方法、路径和协议版本
    if (sscanf(buf, "%9s %99s %19s", method, path, protocol) != 3)
    {
        printf("解析失败\n");
        return;
    }

    // printf("方法:2 %s\n", method);
    printf("路径: %s\n", path);
    // printf("协议版本: %s\n", protocol);

    // 处理get请求
    if (strcasecmp(method, "GET") == 0)
    {
        char *filepath = NULL;
        //
        if (path[1] == '/' && path[0] == '/')
            filepath = path + 2; // 跳过//
        else if (path[1] != '/' && path[0] == '/')
            filepath = path + 1; // 跳过/

        // 转码
        strdecode(filepath, filepath);
        // 判断文件在不在 不能用file==null 因为路径可能为空格 ‘ ’而非‘’
        if (*filepath == 0)
        {
            printf("is null\n");
            filepath = "./";
            // strcpy(filepath, "./");
        }

        printf("filepath: %s\n", filepath);

        struct stat s;
        // strfile = "a.html";
        int ret = stat(filepath, &s);
        if (ret < 0)
        {
            printf("file not found\n");
            send_header(ev->data.fd, 404, "NOT FOUND", get_mime_type("*.html"), 0);
            send_file(ev->data.fd, "err.html", ev, epfd, CLOSE_EV);
        }
        else if (ret == 0)
        {

            // 如果是普通文件
            if (S_ISREG(s.st_mode))
            {
                printf("request is a file\n");
                send_header(ev->data.fd, 200, "OK", get_mime_type(filepath), s.st_size);
                send_file(ev->data.fd, filepath, ev, epfd, CLOSE_EV);
            }
            else if (S_ISDIR(s.st_mode))
            {
                printf("request is a dir\n");
                send_header(ev->data.fd, 200, "OK", get_mime_type("*.html"), 0);

                // 发送header.html
                send_file(ev->data.fd, "./http_res1/dir_header.html", ev, epfd, NOT_CLOSE_EV);

                struct dirent **dirlist = NULL;

                int n = scandir(filepath, &dirlist, NULL, alphasort);
                int len = 0;
                for (int i = 0; i < n; i++)
                {
                    // 为子目录时、在超链接后面加/ 点击时就会自动解析为子目录的资源 /res1 当加上/后就会给访问的超链接加上 /res1/res2
                    if (dirlist[i]->d_type == DT_DIR)
                    {
                        printf("subdir: %s\n", dirlist[i]->d_name);
                        len = sprintf(buf, "<li><a href =%s/ >%s</a></li>", dirlist[i]->d_name, dirlist[i]->d_name);
                    }
                    else
                    {
                        len = sprintf(buf, "<li><a href =%s > %s </a></li>", dirlist[i]->d_name, dirlist[i]->d_name);
                    }
                    send(ev->data.fd, buf, len, 0);
                    free(dirlist[i]);
                }
                free(dirlist);
            }
        }
    }
}

int main()
{
    // 忽略sig_ign信号、以防服务器应写大文件宕机
    signal(SIGPIPE, SIG_IGN);
    // 切换工作目录
    // 获取当前目录路径
    char pwd_path[256];
    char *path = getenv("PWD");
    strcpy(pwd_path, path);
    strcat(pwd_path, "/res");
    chdir(pwd_path);
    printf("%s\n", pwd_path);

    int lfd = tcp4bind(PORT, NULL);
    int op = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op));

    Listen(lfd, 128);
    int epfd = epoll_create1(0);
    struct epoll_event ev;
    ev.data.fd = lfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    struct epoll_event evs[maxn];
    printf("ctl ok\n");

    while (1)
    {
        int nready = epoll_wait(epfd, evs, maxn, 0);
        if (nready < 0)
        {
            perror("");
            return -1;
        }
        for (int i = 0; i < nready; i++)
        {
            for (int i = 0; i < nready; i++)
            {
                if (evs[i].data.fd == lfd && evs[i].events & EPOLLIN)
                {
                    struct sockaddr_in cliaddr;
                    char ip[19] = "";
                    socklen_t len = sizeof(cliaddr);
                    int cfd = Accept(lfd, (struct sockaddr *)&cliaddr, &len);

                    // 非阻塞的已连接套机子
                    int flg = fcntl(cfd, F_GETFL);
                    flg |= O_NONBLOCK;
                    fcntl(cfd, F_SETFL, flg);
                    int op = 1;
                    setsockopt(cfd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op));
                    printf("new conneciton %s:%d \n", inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, ip, 19), ntohs(cliaddr.sin_port));
                    ev.data.fd = cfd;
                    ev.events = EPOLLIN;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
                }
                else if (evs[i].events & EPOLLIN)
                {
                    read_client_request(epfd, &evs[i]);
                }
            }
        }
    }

    return 0;
}
