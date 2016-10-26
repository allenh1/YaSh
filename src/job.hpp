#ifndef __JOB_HPP__
#define __JOB_HPP__
#include <sys/types.h>
#include <sys/wait.h>

enum job_status
{
   STOPPED, /* job got SIGTSTP */
   RUNNING, /* job is running  */
   EXITED   /* job exited normally */
};

struct job
{
   pid_t pgid;

   int stdin;
   int stdout;
   int stderr;

   job_status status;
};
#endif
