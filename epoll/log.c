#include "log.h"

static int msgopt, wanopt; // 定义消息和警告的选项变量

static char msgdatefmt[100], wandatefmt[100], ident_name[100]; // 定义消息和警告日期的时间格式变量、以及程序标识符变量

static struct timeval be_stime; // 定义程序开始的时间变量

static FILE *msgfile = NULL, *wanfile = NULL; // 定义消息和警告日志文件指针

/**
 * @brief 日志文件初始化函数、也可以通过msgLogOpen进行初始化
 */
int msgInit(char *pName)
{
    // 如果成功打开日志文件、则设置日志格式
    if (msgLogOpen(pName, LOG_MESSAGE_FILE, LOG_POSTFIX_MESS, LOG_WARNING_FILE, LOG_POSTFIX_WARN) == 0)
    {
        // 设置日志格式
        msgLogFormat(LOG_PROCNAME | LOG_PID, LOG_MESSAGE_DFMT, LOG_PROCNAME | LOG_PID, LOG_WARNING_DFMT);
    }
    else
    { // 如果打开日志文件失败，则输出错误信息
        printf("can not create log!\n");
        return -1;
    }
    return 0;
}

/**
 * @brief 打开日志文件函数
 * @param ident 标识符，用于标识程序或模块的名称，将在日志记录中作为前缀显示
 * @param mpre 消息日志文件的前缀。这是指定消息日志文件名的一部分，通常包含目录路径和文件名前缀
 * @param mdate 消息日志文件的日期格式。该参数指定了在生成消息日志文件名时使用的日期格式
 */
int msgLogOpen(char *ident, char *mpre, char *mdate, char *wpre, char *wdate)
{
    time_t now_time;                         // 当前时间变量
    char openfilename[200], timestring[100]; // 定义日志文件名、时间字符串变量

    now_time = time(NULL);
    if ((!msgfile) && (*mpre)) // 如果消息日志文件指针为空且指定了消息日志文件名前缀
    {
        strcpy(openfilename, mpre); // 复制消息日志文件名前缀
        if (*mdate)                 // 如果指定了消息日志文件日期格式
        {
            strftime(timestring, sizeof(timestring), mdate, localtime(&now_time));
            strcat(openfilename, ".");
            // 连接日志文件名、日期时间字符串
            strcat(openfilename, timestring);
        }
        // 打开消息日志文件
        if ((msgfile = fopen(openfilename, "a+b")) == NULL) // 参数 "a+b" 表示以追加模式打开文件，如果文件不存在则创建，如果文件存在则从文件末尾开始写入；同时以二进制模式打开文件，用于处理非文本文件。
        {
            printf("openfilename :%s \n", openfilename);
            return -1;
        }
        sendlineBuf(msgfile); // 设置消息日志文件为行缓冲模式
    }
    if (!wanfile && (*wpre)) // 如果告警日志文件指针为空且指定了告警日志文件名前缀
    {
        strcpy(openfilename, wpre); // 复制告警日志文件名前缀
        if (*wdate)                 // 如果指定了告警日志文件日期格式
        {
            strftime(timestring, sizeof(timestring), wdate, localtime(&now_time)); // 格式化当前日期时间
            strcat(openfilename, ".");                                             // 连接日志文件名和日期时间字符串
            strcat(openfilename, timestring);
        }
        if ((wanfile = fopen(openfilename, "a+b")) == NULL) // 打开告警日志文件
        {
            return -1; // 返回错误
        }
        setlinebuf(wanfile); // 设置告警日志文件为行缓冲模式
    }
    // 如果消息和告警日志文件均已打开，则设置默认输出选项和日期时间格式
    if ((msgfile) && (wanfile))
    {
        if (*ident)
        {
            strcpy(ident_name, ident); // 复制程序标识符
        }
        else
        {
            ident_name[0] = '\0';
        }
        msgopt = LOG_PROCNAME | LOG_PID; // 设置默认消息日志输出信息选项 程序名+进程号
        wanopt = LOG_PROCNAME | LOG_PID;

        strcpy(msgdatefmt, "%m-%d %H:%M:%S"); // 设置默认消息日志输出时间格式
        strcpy(wandatefmt, "%m-%d %H:%M:%S"); // 设置默认告警日志输出时间格式

        // 输出消息日志、提示日志文件已经打开
        msglog(MSG_INFO, "File is msgfile = [%d] ,wanfile=[%d].", fileno(msgfile), fileno(wanfile));
        return 0;
    }
    else
    {
        return -1;
    }
}

/**
 * @brief 将可选的信息拼接到日志前缀信息中
 */
void strcatOnlogmsg(char *logprefix, char *tmpstring, struct time_t *curtime)
{ // 将当前时间按照消息日志日期时间格式格式化，并添加程序标识符和进程ID到日志前缀
    strftime(logprefix, sizeof(logprefix), msgdatefmt, localtime(curtime));
    strcat(logprefix, " ");
    if (msgopt & LOG_PROCNAME)
    {
        strcat(logprefix, ident_name);
        strcat(logprefix, " ");
    }
    if (msgopt & LOG_PID)
    {
        sprintf(tmpstring, "[%6d]", getpid());
        strcat(logprefix, tmpstring);
    }
}

/**
 * @brief 自定义日志输出函数、可以按照普通信息及警告分类输出程序志.
 * @param outfmt 以outfmt的格式写进日志文件中、功能类似fprintf
 */
int msglog(int mtype, char *outfmt, ...)
{
    struct time_t now_time;
    va_list ap;                            // 定义变参列表
    char logprefix[1024], tmpstring[1024]; // 定义日志前缀和临时字符串变量

    time(&now_time);
    if (mtype & MSG_INFO) // 普通信息
    {
        strcatOnlogmsg(logprefix, tmpstring, &now_time);
        // 将日志前缀和格式化的日志信息写入到消息日志文件
        fprintf(msgfile, "%s: ", logprefix);

        // 开始迭代变参列表
        va_start(ap, outfmt);
        vfprintf(msgfile, outfmt, ap);
        va_end(ap);
        fprintf(msgfile, "\n");
    }
    if (mtype & MSG_WARN) // 警告信息
    {
        strcatOnlogmsg(logprefix, tmpstring, &now_time);
        // 将日志前缀和格式化的日志信息写入告警日志文件
        fprintf(wanfile, "%s: ", logprefix);
        va_start(ap, outfmt);
        vfprintf(wanfile, outfmt, ap);
        va_end(ap);
        fprintf(wanfile, "\n");
    }
    return 0;
}

/**
 * @brief 设置日志格式及选项函数
 * @param mopt 消息日志选项
 * @param mdfmt 消息日志日志格式化模板
 */
int msgLogFormat(int mopt, char *mdfmt, int wopt, char *wdfmt)
{
    // 传入了指定格式的消息日志选项 mopt不为空
    if (mopt >= 0)
    {
        msgopt = mopt;
    }
    else
    {
        msgopt = msgopt & mopt; // 为空、将msgopt也置空
    }

    if (wopt >= 0)
    {
        wanopt = wopt;
    }
    else
    {
        wanopt = wanopt & wopt;
    }

    if (*mdfmt)
    {
        strcpy(msgdatefmt, mdfmt);
    }

    if (*wdfmt)
    {
        strcpy(wandatefmt, wdfmt);
    }
    return 0;
}

/**
 * @brief 关闭日志文件函数
 */
int msgLogClose()
{
    if (msgfile)
    {
        fclose(msgfile); // 关闭消息日志文件
    }
    if (wanfile)
        fclose(wanfile);
    return 0;
}

/**
 * @brief 设置开始时间函数、保存时间到be_stime中
 */
long begusec_prcess()
{
    gettimeofday(&be_stime, NULL);
    return 0;
}

long getusec_process()
{
    struct timeval ed_stime; // 定义结束时间的变量

    gettimeofday(&ed_stime, NULL);

    // 返回当前时间与开始时间的插值、单位为微秒
    return ((ed_stime.tv_sec - be_stime.tv_sec) * 1000000 + ed_stime.tv_usec - be_stime.tv_usec);
}
