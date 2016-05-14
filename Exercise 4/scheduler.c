#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */

/* Global Variables. */
int *childid, *alive, current, nprog;

/*
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum)
{
	//assert(0 && "Please fill me!");
	kill(childid[current], SIGSTOP);
}

/* 
 * SIGCHLD handler
 */
static void
sigchld_handler(int signum)
{
	int alldead, i;	
	pid_t p;
	int status;
	for(;;){
		//assert(0 && "Please fill me!");
		/* 
		 * waitpid : on success, returns the process ID of the child whose  state has changed
		 * -1 : wait for any child proccess
		 * WNOHANG : return immediately if no child has exited.
		 * WUNTRACED : also return if a child has  stopped  (but  not  traced  via ptrace(2))
		*/
		p = waitpid(-1, &status, WUNTRACED | WNOHANG);
		//explain_wait_status(p, status);
		if(p==0)
			break;
		childid[current] = (int) p;
		if (WIFEXITED(status)  || WIFSIGNALED(status)) {
			/* A child has died */
			printf("Parent: Received SIGCHLD, child %d is dead.\n", childid[current]);
			alive[current] = 0;
		}
		if (WIFSTOPPED(status)) {
			/* A child has stopped due to SIGSTOP/SIGTSTP, etc... */
			printf("Parent: Child %d has been stopped. Moving right along...\n", childid[current]);
		}
		printf("Moving on to ");

		alldead = 1;
		for (i = 0; i < nprog; i++){
			if (alive[i] == 1)
				alldead = 0;
		}
		if (alldead == 1) {
			printf("All children dead.\n");
			exit(0);
		}	
	
		do{
			current = ((current+1)%nprog);
		}while (alive[current] == 0);
		printf("%d.\n", current);
		alarm(SCHED_TQ_SEC);
		kill(childid[current], SIGCONT); 
	}
}

/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers(void)
{
	sigset_t sigset;
	struct sigaction sa;

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigaddset(&sigset, SIGALRM);
	sa.sa_mask = sigset;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction: sigchld");
		exit(1);
	}

	sa.sa_handler = sigalrm_handler;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		perror("sigaction: sigalrm");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE, so that write()s to pipes
	 * with no reader do not result in us being killed,
	 * and write() returns EPIPE instead.
	 */
	if (signal(SIGPIPE, SIG_IGN) < 0) {
		perror("signal: sigpipe");
		exit(1);
	}
}

static void
do_child(char *executable)
{
	char *newargv[] = { executable , NULL, NULL, NULL};
	char *newenviron[] = { NULL };

	raise(SIGSTOP);
	execve( executable , newargv, newenviron);
	/* execve() only returns on error */
	perror("scheduler: child: execve");
	exit(1);
}

int main(int argc, char *argv[])
{
	pid_t p;
	int i;
	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */
	current = 0; /* current proccess number goes here */
	nprog = argc - 1; /* number of proccesses goes here */
	childid = malloc((nprog)*sizeof(int));
	alive = malloc((nprog)*sizeof(int));
	for (i=0; i < nprog; i++){
		printf("forking..%d %d\n", argc, i);
		p = fork();
		//if (p<0)...
		if (p == 0){
			do_child(argv[i+1]);
		}
		else{
			childid[i] = (int) p;
			alive[i] = 1;
		}
	}

	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nprog);
	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	if (nprog == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}
	else{
		alarm(SCHED_TQ_SEC);	
		kill(childid[0], SIGCONT);
		
	}


	/* loop forever  until we exit from inside a signal handler. */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
