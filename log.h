#ifndef LOG_H
#define LOG_H

#include <dirent.h>     // 目录操作相关头文件
#include <errno.h>      // 错误码定义头文件
#include <fcntl.h>      // 文件控制相关头文件
#include <math.h>       // 数学库头文件
#include <signal.h>     // 信号处理相关头文件
#include <stdarg.h>     // 可变参数列表相关头文件
#include <stdio.h>      // 标准输入输出库头文件
#include <stdlib.h>     // 标准库头文件
#include <string.h>     // 字符串操作相关头文件
#include <sys/ipc.h>    // IPC相关头文件
#include <sys/sem.h>    // 信号量相关头文件
#include <sys/shm.h>    // 共享内存相关头文件
#include <sys/stat.h>   // 文件状态相关头文件
#include <sys/statfs.h> // 文件系统信息相关头文件
#include <sys/types.h>  // 系统数据类型定义头文件
#include <time.h>       // 时间操作相关头文件
#include <sys/time.h>   // 时间操作相关头文件
#include <unistd.h>     // POSIX操作系统API头文件

// 定义日志输出选项
#define LOG_PROCNAME 0x00000001 // 输出日志时打印程序名
#define LOG_PID 0x00000010      // 输出日志时打印进程ID
#define LOG_PERROR 0x00000100   // 将澳错误信息输出到标准错误流

// 定义日志文件名后缀选项
#define NLO_PROCNAME 0x11111110 // 保留程序名作为日志文件名后缀
#define NLO_PID 0x11111101      // 保留进程ID作为日志文件名后缀
#define NLO_PERROR 0x11111011   // 保留错误信息作为日志文件名后缀

// 定义日志类型选项
#define MSG_INFO 0x00000001          // 普通信息
#define MSG_WARN 0x00000010          // 告警信息
#define MSG_BOTH MSG_INFO | MSG_WARN // 普通信息和告警信息

// 定义日志文件路径和格式
#define LOG_MESSAGE_FILE "/home/itheima/log/tcpsvr"       // 消息日志文件路径
#define LOG_MESSAGE_DFMT "%m-%d %H:%M:%S"                 // 消息日志时间格式
#define LOG_POSTFIX_MESS "%y%m"                           // 消息日志文件名后缀
#define LOG_WARNING_FILE "/home/itheima/log/log.sys_warn" // 告警日志文件路径
#define LOG_WARNING_DFMT "%m-%d %H:%M:%S"                 // 告警日志时间格式
#define LOG_POSTFIX_WARN ""                               // 告警日志文件名后缀

// 函数声明
int msglog(int mtype, char *outfmt, ...);                                      // 写日志函数
int msgLogFormat(int mopt, char *mdfmt, int wopt, char *wdfmt);                // 设置日志格式函数
int msgLogOpen(char *ident, char *mpre, char *mdate, char *wpre, char *wdate); // 打开日志文件函数
int msgLogClose(void);                                                         // 关闭日志文件函数
long begusec_process(void);                                                    // 设置开始时间函数
long getusec_process(void);                                                    // 获取程序运行时间函数
int msgInit(char *pName);                                                      // 日志初始化函数

#endif
