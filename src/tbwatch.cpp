
#include "tbwatch.h"
#include <string.h>

#define WATCH_SEPARATOR ':'

//signal handler must be static (see term_handler)
//  these system resources need to be available 
//  when process is killed, either by user or by 
//  reboot/shutdown
std::vector<watch_path_t>  TbWatch::g_watch_info;

int TbWatch::g_unixsock_fd = 0;
std::string TbWatch::g_sockpath = "/run/tbwatch.socket";

//PUBLIC

/**************************
 *
 * constructor
 *
 **************************/
TbWatch::TbWatch(std::vector<std::string>& paths) : m_tbpaths(paths)
{
    m_mypid = getpid();
    register_sighandler();
    setCurrentDirectory();
    //opensocket();
    createEventMap();
    parsePathEvents();
}


/**************************
 *
 * start the watches 
 *
 **************************/
void
TbWatch::startWatches()
{
    int inotifyFd;
    char buf[BUF_LEN] __attribute__ ((aligned(8)));
    ssize_t numRead;
    char *p;
    struct inotify_event *event;

    inotifyFd = inotify_init();         /* Create inotify instance */
    if (inotifyFd == -1) {
        std::cerr << "ERROR: inotify_init" << getErrnoString() << std::endl;
        exit(EXIT_FAILURE);
    }

    //Got instance, try to add watches
    std::string timestamp = getTimeStamp();
    for (std::vector<watch_path_t>::iterator it=g_watch_info.begin(); it != g_watch_info.end(); ++it) {
        it->wd = inotify_add_watch(inotifyFd, it->pathname.c_str(), it->eventmask);
        if (-1 == it->wd) {
            std::cerr << timestamp << " TbWatch: Error adding watch for: " << it->pathname << " eventmask: " << it->eventmask << " :" << getErrnoString() << std::endl;
            std::cerr <<  timestamp << " Process " << m_mypid << " exiting" << std::endl;
            exit(EXIT_FAILURE);
        } 
    } 


    for (;;) { /* Read events forever */
        numRead = read(inotifyFd, buf, BUF_LEN);
        if (numRead <= 0) {
            std::cerr << getTimeStamp() << "ERROR: read() from inotify fd returned <= 0! : " << getErrnoString() << std::endl;
            exit(EXIT_FAILURE);
        }

        /* Process all of the events in buffer returned by read() */
        for (p = buf; p < buf + numRead; ) {
            event = (struct inotify_event *) p;
            logevent(event);
            p += sizeof(struct inotify_event) + event->len;
        }
    }
}


//PRIVATE
void
TbWatch::opensocket() const 
{
    if (0 > (g_unixsock_fd = socket(PF_UNIX, SOCK_DGRAM, 0))) {
        std::cerr << "Tbwatch: failed to get socket descriptor\n";
        return;
    }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, TbWatch::g_sockpath.c_str(), sizeof(TbWatch::g_sockpath.c_str()));
    bind(TbWatch::g_unixsock_fd, (struct sockaddr*)&addr, sizeof(addr));
}

void
TbWatch::setCurrentDirectory() const
{
    int ret = chdir(TBDIR);
    if (-1 == ret) {    
        std::cerr << getTimeStamp() << " Could not change dir to : " << TBDIR << std::endl;
        cleanup();
        exit(EXIT_FAILURE);
    }
}


/**************************
 *
 * get errno string
 *
 **************************/
std::string
TbWatch::getErrnoString()
{
    char *errbuf = 0;
    //GNU version will likely be used, and it will not necessarily copy the string
    //   to the provided buffer!  But, it will return the char pointer
    errbuf = strerror_r(errno, m_errbuf, ERRBUFSZ);
    if (errbuf) {
        m_errnostring.assign((const char *)errbuf);
    }
    else {
        m_errnostring.assign(" COULD NOT GET ERROR");
    }
    return m_errnostring;
}


/**************************
 *
 * term_handler
 *
 **************************/
void
TbWatch::term_handler(int signo)
{
    if (signo == SIGTERM) {
        //when called at reboot/shutdown, only so much time is allowed, so be quick about it :)
        for(std::vector<watch_path_t>::iterator it = g_watch_info.begin(); it != g_watch_info.end(); ++it) {
            if (it->wd > 0)
                close(it->wd); 
        }

        exit(EXIT_SUCCESS);
    }
}

/**************************
 *
 * register sighandler
 *
 **************************/
void
TbWatch::register_sighandler() const
{
    if (signal(SIGTERM, TbWatch::term_handler) == SIG_ERR) {
        std::cerr << getTimeStamp() << "tbwatch: Failed to register signal handler: PID: " << m_mypid << " exiting" << std::endl;
        exit(EXIT_FAILURE);
    }
}


/**************************
 *
 * log event msg
 *
 **************************/
void
TbWatch::logevent(const struct inotify_event *evnt) const
{
    std::stringstream logstream;
    /* Display information from inotify_event structure */
    std::string timestamp = getTimeStamp();
    logstream << timestamp << ": ";
 
    std::string drname;
    if (evnt->cookie > 0) { //cookes link together IN_MOVED_FROM and IN_MOVED_TO
        //std::cout << timestamp << " cookie = " << evnt->cookie << std::endl;
        logstream << " cookie: " << evnt->cookie;
    }

    for (size_t j=0; j<m_tbpaths.size(); j++) {
         
        if (g_watch_info[j].wd == evnt->wd) {
             
            drname = g_watch_info[j].pathname;
            //std::cout << timestamp << " : watch descriptor: " << evnt->wd << ",path: " << drname << std::endl;
            logstream <<  ": watch descriptor: " << (int)evnt->wd << " :path=" << drname; 
        }
    } 

    if (evnt->mask & IN_ACCESS)        logstream << " IN_ACCESS ";
    if (evnt->mask & IN_ATTRIB)        logstream << " IN_ATTRIB ";
    if (evnt->mask & IN_CLOSE_NOWRITE) logstream << " IN_CLOSE_NOWRITE ";
    if (evnt->mask & IN_CLOSE_WRITE)   logstream << " IN_CLOSE_WRITE ";
    if (evnt->mask & IN_CREATE)        logstream << " IN_CREATE ";
    if (evnt->mask & IN_DELETE)        logstream << " IN_DELETE ";
    if (evnt->mask & IN_DELETE_SELF)   logstream << " IN_DELETE_SELF ";
    if (evnt->mask & IN_IGNORED)       logstream << " IN_IGNORED ";
    if (evnt->mask & IN_ISDIR)         logstream << " IN_ISDIR ";
    if (evnt->mask & IN_MODIFY)        logstream << " IN_MODIFY ";
    if (evnt->mask & IN_MOVE_SELF)     logstream << " IN_MOVE_SELF ";
    if (evnt->mask & IN_MOVED_FROM)    logstream << " IN_MOVED_FROM ";
    if (evnt->mask & IN_MOVED_TO)      logstream << " IN_MOVED_TO ";
    if (evnt->mask & IN_OPEN)          logstream << " IN_OPEN ";
    if (evnt->mask & IN_Q_OVERFLOW)    logstream << " IN_Q_OVERFLOW ";
    if (evnt->mask & IN_UNMOUNT)       logstream << " IN_UNMOUNT ";

    if (evnt->len > 0) //len may be larger than strlen(name)
    //    std::cout << timestamp << ": " << " name = " << evnt->name << std::endl;
        logstream <<  ": " << " name = " << evnt->name << std::endl;
    else
        logstream << std::endl;

    //put to syslog (via systemd)
    std::cout << logstream.str() << std::flush;
}


/**************************
 *
 * getTimeStamp
 *
 **************************/
std::string
TbWatch::getTimeStamp() const
{
    std::string timestring;
    char timebuf[TIMEBUFSZ];
    time_t thetime = time(NULL);

    struct tm * mytime = localtime(&thetime);
    int numBytes = strftime(timebuf, TIMEBUFSZ, "%Y-%m-%d %H:%M:%S", mytime);

    if (0 < numBytes) {
        timestring = std::string(timebuf);
    }
    else {
        timestring = std::string("NULL_TIME");
    }
        
    return timestring;    
}

bool
TbWatch::parsePathEvents()
{
    //parse the m_tbpaths to see if events were specified as part of the path
    //for instance the config file may have specified:
    //   watch=/path/to/watch:IN_OPEN:IN_MODIFY:IN_DELETE
    for (std::vector<std::string>::iterator it = m_tbpaths.begin(); it != m_tbpaths.end(); ++it) {
        bool firstsep = false;
        size_t lastpos = 0;
        watch_path_t wpath;
        std::vector<std::string> ev_vect;
        for (size_t i=0; i<it->length(); ++i) {
            if (WATCH_SEPARATOR == it->at(i)) {
                if (false == firstsep) {
                    firstsep = true;
                    std::string path = it->substr(0,i);
                    wpath.pathname = path;
                    lastpos = i;  
                }
                else {
                    //std::string::size_type pos = it->find(":");
                    std::string ev = it->substr(lastpos+1, i-lastpos-1);
                    ev_vect.push_back(ev);
                    lastpos = i;
                }
            }
        }
        //if firstsep is still false, then the default case
        if (false == firstsep) {
            wpath.pathname = *it;
            wpath.eventmask = IN_CREATE | IN_DELETE | IN_MODIFY;
        }
        else {
            //get the last of the events
            std::string ev = it->substr(lastpos+1);
            ev_vect.push_back(ev);
            wpath.eventmask = 0;
            for (std::vector<std::string>::iterator it=ev_vect.begin(); it != ev_vect.end(); ++it) {
                std::unordered_map<std::string, size_t>::iterator eventMapIter = m_event_map.find(*it);
                if (eventMapIter != m_event_map.end()) {
                    wpath.eventmask |= eventMapIter->second; 
                }    
                else {
                    std::cerr << "TbWatch::parsePathEvents: " << *it << " is an invalid event! " << std::endl;
                    std::cerr << "Tbwatch process " << m_mypid << " exiting" << std::endl;
                    cleanup();
                    exit(EXIT_FAILURE);
                }
            }
        }
        //finally, push the watch info on the global watch list
        g_watch_info.push_back(wpath); 
    }
    
    return true;
}

/**************************
 *
 * create event map
 *
 **************************/
bool
TbWatch::createEventMap()
{
    //Primary events
    m_event_map["IN_ACCESS"] = IN_ACCESS; 
    m_event_map["IN_MODIFY"] = IN_MODIFY; 
    m_event_map["IN_ATTRIB"] = IN_ATTRIB; 
    m_event_map["IN_CLOSE_WRITE"] = IN_CLOSE_WRITE; 
    m_event_map["IN_CLOSE_NOWRITE"] = IN_CLOSE_NOWRITE; 
    m_event_map["IN_OPEN"] = IN_OPEN; 
    m_event_map["IN_MOVED_FROM"] = IN_MOVED_FROM; 
    m_event_map["IN_MOVED_TO"] = IN_MOVED_TO; 
    m_event_map["IN_CREATE"] = IN_CREATE; 
    m_event_map["IN_DELETE"] = IN_DELETE; 
    m_event_map["IN_DELETE_SELF"] = IN_DELETE_SELF; 
    m_event_map["IN_MOVE_SELF"] = IN_MOVE_SELF; 
    //helpers
    m_event_map["IN_CLOSE"] = (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE); 
    m_event_map["IN_MOVE"] = (IN_MOVED_FROM | IN_MOVED_TO); 
    m_event_map["IN_ALL_EVENTS"] = IN_ALL_EVENTS; 

    return true;
}


/**************************
 *
 * cleanup
 *
 **************************/
void
TbWatch::cleanup() const
{
    for(std::vector<watch_path_t>::iterator it = g_watch_info.begin(); it != g_watch_info.end(); ++it) {
        if (it->wd > 0)
            close(it->wd); 
    }
}


/**************************
 *
 * destructor
 *
 **************************/
TbWatch::~TbWatch()
{
    //if we need to exit for an error condition, we will call cleanup
}

/* HELP
 IN_ACCESS          File was read from
 IN_ATTRIB          File's metadata (inode or xattr) was changed
 IN_CLOSE_NOWRITE   File was closed (and was not open for writing)
 IN_CLOSE_WRITE     File was closed (and was open for writing)
 IN_CREATE          File was created
 IN_DELETE          File was deleted
 IN_DELETE_SELF     The watch itself was deleted
 IN_IGNORED         ?
 IN_ISDIR           ?
 IN_MODIFY          File was written to 
 IN_MOVE_SELF       The watch was moved
 IN_MOVED_FROM      File was moved away from watch
 IN_MOVED_TO        File was moved to watch
 IN_OPEN            File was opened
 IN_Q_OVERFLOW      The inotify queue overflowed
 IN_UNMOUNT         Watch was unmounted

**helpers
  IN_CLOSE      = IN_CLOSE_WRITE | IN_CLOSE_NOWRITE
  IN_MOVE       = IN_MOVED_FROM | IN_MOVED_TO
  IN_ALL_EVENTS = Bitwise OR of all events
*/
 
