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

	void restore_io() {
		dup2(stdin,  0); close(stdin);
		dup2(stdout, 1); close(stdout);
		dup2(stderr, 2); close(stderr);
	}

	job_status status;
};
#endif
