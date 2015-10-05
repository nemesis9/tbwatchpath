#ifndef _TBWATCH_H
#define _TBWATCH_H

#include <limits.h>      //NAME_MAX
#include <stdlib.h>      //exit, EXIT_FAILURE
#include <unistd.h>      //fork, setsid
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <time.h>
#include <string>
#include <string.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map> //C++11

//#include <iomanip>

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
#define TIMEBUFSZ 64
#define ERRBUFSZ 128

#define TBDIR "/run"

typedef struct 
{
   int wd;                   //watch descriptor
   std::string pathname;     //watch path
   uint32_t eventmask;       //active watch events
} watch_path_t;


class TbWatch 
{
    public:
        TbWatch(std::vector<std::string>& tbpathlist);
        ~TbWatch();

        void startWatches();

    private:
        void setCurrentDirectory() const;
        pid_t m_mypid;

        char m_errbuf[ERRBUFSZ];
        std::string m_errnostring;
        std::string getErrnoString();

        //static resources that need to be
        //available to the static SIGTERM handler
        static int g_unixsock_fd;
        static std::string g_sockpath;
        void opensocket() const;
        static std::vector<watch_path_t> g_watch_info;
       
        //static SIGTERM handler
        static void term_handler(int signo);

        //functions used in startup
        void register_sighandler() const;

        //vars/functions used at runtime
        void logevent(const struct inotify_event *event) const;
        std::string getTimeStamp() const;
        void cleanup() const;

        std::vector<std::string> m_tbpaths;
        std::unordered_map<std::string, size_t> m_event_map;
        bool createEventMap();
        bool parsePathEvents();
};


#endif //_TBWATCH_H

