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

	int m_stdin;
	int m_stdout;
	int m_stderr;

	std::string command;

	/* @todo do we still need this? */
	void restore_io() {
		dup2(m_stdin,  0); close(m_stdin);
		dup2(m_stdout, 1); close(m_stdout);
		dup2(m_stderr, 2); close(m_stderr);
	}

	job_status status;
};
#endif
