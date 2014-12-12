/**
 * Project Clearwater - IMS in the Cloud
 * Copyright (C) 2014 Metaswitch Networks Ltd
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version, along with the "Special Exception" for use of
 * the program along with SSL, set forth below. This program is distributed
 * in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details. You should have received a copy of the GNU General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * The author can be reached by email at clearwater@metaswitch.com or by
 * post at Metaswitch Networks Ltd, 100 Church St, Enfield EN2 6BQ, UK
 *
 * Special Exception
 * Metaswitch Networks Ltd grants you permission to copy, modify,
 * propagate, and distribute a work formed by combining OpenSSL with The
 * Software, or a work derivative of such a combination, even if such
 * copying, modification, propagation, or distribution would otherwise
 * violate the terms of the GPL. You must comply with the GPL in all
 * respects for all of the code used other than OpenSSL.
 * "OpenSSL" means OpenSSL toolkit software distributed by the OpenSSL
 * Project and licensed under the OpenSSL Licenses, or a work based on such
 * software and licensed under the OpenSSL Licenses.
 * "OpenSSL Licenses" means the OpenSSL License and Original SSLeay License
 * under which the OpenSSL Project distributes the OpenSSL toolkit software,
 * as those licenses appear in the file LICENSE-OPENSSL.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <net-snmp/library/snmp_assert.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/library/container.h>
#include <net-snmp/agent/table_array.h>
#ifdef __cplusplus
}
#endif

#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/prctl.h>
#include <netdb.h>

#define AGENTX_UNIX 0

#define __NAMER__(x, y) x##y
#define NAMER(x, y) __NAMER__(x, y)

extern "C" void NAMER(init_, AGENTX_NAME)();

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

// Note: Throughout this code you'll see - kill(getpid(), SIGKILL). This is done for a reason,
// that being that we don't want any exit handlers called, so we SIGKILL ourselves.

extern "C" {
  static netsnmp_session* nsess=0;
  static struct snmp_session Session;

  // Agentx event processing thread
  static void* snmp_event_thread(void* notused)
  {
    while(true)
    {
      agent_check_and_process(1);
    }
    return NULL;
  }

  //
  // This function is a kludge looking for errors connecting to SNMPD
  // so that when we see one we just exit and recover. There's no good
  // reason for us not to be able to connect, but it does happen periodically
  // usually because there was an EACCES error opening the agentx named
  // socket.It is unknown why we sometimes get an EACCES error...
  //
  static int agentx_handler_stdouterr(netsnmp_log_handler* logh, int pri, const char* str)
  {
    if (pri == LOG_WARNING)
    {
      static const char* warn="Warning: Failed to connect to the agentx master agent";
      static int wlen=strlen(warn);
      if (strncmp(str, warn, wlen) == 0)
      {
        static int cnt=0;

        if (cnt >= 0)
        {
          snmp_log(LOG_ERR, "%s %s:%d can't connect to agentx master socket (%d)! exiting...\n", STR(AGENTX_NAME), __FILE__,  __LINE__, cnt);
          kill(getpid(), SIGKILL);
        }
        cnt++;
      }
    }

    return 1;
  }

  static void NAMER(AGENTX_NAME, _init)()
  {
    snmp_enable_syslog_ident(STR(AGENTX_NAME), LOG_DAEMON);
    netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_DONT_READ_CONFIGS, 1);
    //netsnmp_ds_set_string(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_X_SOCKET, "/var/agentx/master");
    netsnmp_ds_set_string(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_X_SOCKET, "tcp:localhost:705");
    netsnmp_ds_set_string(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_COMMUNITY, "clearwater");

    SOCK_STARTUP;

    static netsnmp_log_handler* logh=netsnmp_register_loghandler(NETSNMP_LOGHANDLER_STDERR, LOG_WARNING);
    logh->handler=agentx_handler_stdouterr;

    //
    // Wait for the master to start accepting AgentX connections. Periodically, for
    // reasons unknown, our agent can't connect to SNMPD, so this code was added to
    // detect the problem early on and recover.
    //
#if AGENTX_UNIX
    int sock=socket(PF_UNIX, SOCK_STREAM, 0);
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
#endif
    snmp_log(LOG_INFO, "%s %s:%d socket()=%d(%d=%s)...\n", STR(AGENTX_NAME), __FILE__,  __LINE__, sock, errno, strerror(errno));
    int i=0;
    for(i=0; i < 5; i++)
    {
#if AGENTX_UNIX
      struct sockaddr_un addr;
      addr.sun_family=AF_FILE;
      strcpy(addr.sun_path, "/var/agentx/master");
      int csock=connect(sock, (struct sockaddr*)&addr, sizeof(addr));
#else
      struct sockaddr_in addr;
      struct hostent *server;

      server = gethostbyname("localhost");
      bzero((char *) &addr, sizeof(addr));
      addr.sin_family = AF_INET;
      bcopy((char *)server->h_addr, 
            (char *)&addr.sin_addr.s_addr,
            server->h_length);
      addr.sin_port = htons(705);
      int csock=connect(sock, (struct sockaddr *) &addr, sizeof(addr));
#endif
      if (csock == -1 && errno == EACCES)
      {
        snmp_log(LOG_ERR, "%s %s:%d can't access /var/agentx/master! Exiting...\n", STR(AGENTX_NAME), __FILE__,  __LINE__, i);
        kill(getpid(), SIGKILL);
      }
      snmp_log(LOG_INFO, "%s %s:%d connect()=%d(%d=%s)...\n", STR(AGENTX_NAME),
               __FILE__,  __LINE__, csock, errno, strerror(errno));
      close(csock);
      if (csock != -1)
      {
        break;
      }
      sleep(1);
    }
    if (i == 5)
    {
      snmp_log(LOG_ERR, "%s %s:%d timed out connecting to /var/agentx/master! Exiting...\n", STR(AGENTX_NAME), __FILE__,  __LINE__);
      kill(getpid(), SIGKILL);
    }
    close(sock);

    snmp_log(LOG_INFO, "%s %s:%d sess_init...\n", STR(AGENTX_NAME), __FILE__,  __LINE__);
    nsess= &Session;
    snmp_sess_init(nsess);

    snmp_log(LOG_INFO, "%s %s:%d init_agent...\n", STR(AGENTX_NAME), __FILE__,  __LINE__);
    init_agent(STR(AGENTX_NAME));
    snmp_log(LOG_INFO, "%s %s:%d init_snmpd...\n", STR(AGENTX_NAME), __FILE__,  __LINE__);
    init_snmp(STR(AGENTX_NAME));

    snmp_log(LOG_INFO, "%s %s:%d init()...\n", STR(AGENTX_NAME), __FILE__,  __LINE__);
    NAMER(init_, AGENTX_NAME)();
  }

  int main(int argc, char* argv[])
  {
    int gID=getpgrp();
    if (argc > 1 && strcmp(argv[1], "launcher") == 0)
    {
      // If we're in launcher mode, fork & exec ourselves
      // as the Agentx handler. If we're the parent then we'll
      // loop looking for our parent to die and take appropriate action
      // when that happens. I had to do this because upon implementing I could not
      // get the OS to completly kill the process group when the parent dies, so
      // a little extra work was needed.
      pid_t pID = fork();
      if (pID == 0)     // child
      {
        setpgid(getpid(), gID);
        char* argv_[]= {(char*)argv[0], 0};
        char* env_[]= {0};
        int rc=execve(argv[0], argv_, env_);
        fprintf(stderr, "%s execve(%s, ...) failed=%d, errno=%d(%s)\n", STR(AGENTX_NAME), argv[0], rc, errno, strerror(errno));
        kill(getpid(), SIGKILL);
      }
      else if (pID < 0) // failed to fork
      {
        kill(getpid(), SIGKILL);
      }
      else             // parent
      {
        while(true)
        {
          pid_t ppid;
          if ((ppid=getppid()) > 1)
          {
            sleep(1);
            int sta, rc;
            if ((rc=waitpid(pID, &sta, WNOHANG)) != 0)
            {
              if (rc == -1)
              {
                fprintf(stderr, "%s waitpid error errno=%d(%s), sta=%d\n", STR(AGENTX_NAME), errno, strerror(errno), sta);
              }
              else
              {
                if (WIFEXITED(sta))
                {
                  fprintf(stderr, "%s Child ended normally.\n", STR(AGENTX_NAME));
                }
                else if (WIFSIGNALED(sta))
                {
                  fprintf(stderr, "%s Child ended because of an uncaught signal\n", STR(AGENTX_NAME));
                }
                else if (WIFSTOPPED(sta))
                {
                  fprintf(stderr, "%s Child process has stopped\n", STR(AGENTX_NAME));
                }
                fprintf(stderr, "%s child died, rc=%d, sta=%d\n", STR(AGENTX_NAME), rc, sta);
                kill(getpid(), SIGKILL);
              }
            }
          }
          else
          {
            fprintf(stderr, "%s No parent killing %d, getppid()=%d\n", STR(AGENTX_NAME), pID, (int)ppid);
            kill(pID, SIGKILL);
            kill(getpid(), SIGKILL);
          }
        }
      }
    }
    else
    {
      // If in agent mode, we'll run the original handler's code and then start a thread
      // to deal with SNMP events.
      NAMER(AGENTX_NAME, _init)();

      snmp_event_thread(NULL);
    }
    return 0;
  }
}

