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

#define __NAMER__(x, y) x##y
#define NAMER(x, y) __NAMER__(x, y)

#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/prctl.h>

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

// Note: Throughout this code you'll see - kill(getpid(), SIGKILL). This is done for a reason,
// that being that we don't want any exit handlers called, so we SIGKILL ourselves.

extern "C" {

  // This function launches (i.e. forks/execs) the handler process.
  static void* snmp_handler_launch(void* notused)
  {
    int rc=prctl(PR_SET_PDEATHSIG, SIGKILL);
    snmp_log(LOG_INFO, "%s prctl=%d, errno=%d(%s)\n", STR(AGENTX_NAME), rc, errno, strerror(errno));
    int gID=getpgrp();
    snmp_log(LOG_ERR, "%s gID=%d, getpid()=%d\n", STR(AGENTX_NAME), gID, (int)getpid());
    pid_t pID = vfork();
    if (pID == 0)     // child
    {
      // Exec the real handler that will communicate with SNMPd via Agentx.
      snmp_log(LOG_ERR, "%s getppid()=%d\n", STR(AGENTX_NAME), (int)getppid());
      {
        snmp_log(LOG_INFO, "%s prctl=%d, errno=%d(%s)\n", STR(AGENTX_NAME), rc, errno, strerror(errno));
        setpgid(getpid(), gID);
        char* argv[]= {(char*)"/usr/share/clearwater/bin/snmp_" STR(AGENTX_NAME), (char*)"launcher", 0};
        char* env[]= {0};
        snmp_log(LOG_INFO, "%s launching %s %s\n", STR(AGENTX_NAME), argv[0], argv[1]);
        int rc=execve(argv[0], argv, env);
        snmp_log(LOG_INFO, "%s execve(%s, ...) failed=%d, errno=%d(%s)\n", STR(AGENTX_NAME), argv[0], rc, errno, strerror(errno));
        kill(getpid(), SIGKILL);
      }
    }
    else if (pID < 0) // failed to fork
    {
      snmp_log(LOG_ERR, "Failed to fork of %s\n", STR(AGENTX_NAME));
      kill(getpid(), SIGKILL);
    }
    else              // parent
    {
      // Wait for my child to die and when it does, kill ourselves.
      int sta;
      setpgid(pID, gID);
      snmp_log(LOG_ERR, "%s %d wait for pid=%d to exit\n", STR(AGENTX_NAME), (int)getpid(), pID);
      waitpid(pID, &sta, 0);
      snmp_log(LOG_ERR, "%s (pid=%d) exited with status=%d\n", STR(AGENTX_NAME), pID, sta);
      kill(getpid(), SIGKILL);
    }

    return NULL;
  }

  // Shared object entry point.It just starts a thread that does the launching (above).
  void NAMER(init_, AGENTX_NAME)()
  {
    pthread_t tid;
    pthread_create(&tid, NULL, &snmp_handler_launch, (void*)NULL);
  }
}

