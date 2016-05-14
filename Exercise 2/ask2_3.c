#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "proc-common.h"
#include "tree.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

pid_t make_proc_tree(struct tree_node *node)
{
	int i,status;
	pid_t pid, *pid_child;
	pid = fork();
	if (pid<0){	/*Error*/
		printf("%s :",node->name);
		perror("fork");
		exit(-1);
	}
	if (pid==0){
		printf("Name %s, PID = %ld ,starting... \n",node->name,(long)getpid()); /*message for starting*/
		change_pname(node->name);	/*change process name*/
		pid_child = (pid_t *)malloc((node->nr_children)*sizeof(pid_t));
		for (i=0; i<node->nr_children; i++){
			pid_child[i] = make_proc_tree(node->children+i); /*store in an array the pids of children*/
			wait_for_ready_children(1);    /*Every process as a father waits for child to be ready (sigstop) before the next (DFS)*/
		}
		raise(SIGSTOP);	/*then stops until SIGCONT*/
                printf("Name %s, PID = %ld is awake\n",node->name,(long)getpid());	/*SIGCONT - get's awake - message*/
		for (i=0; i<node->nr_children; i++) {
			pid = pid_child[i];
			kill(pid,SIGCONT);	/*sends a SIGCONT message to every child*/
			pid = wait(&status);	/*then waits for the child to terminate*/
			explain_wait_status(pid, status);
		}
		printf("Name %s, PID = %ld, exiting... \n",node->name,(long)getpid());	/*and then exit*/
		exit(0);
		}
	return pid;
}

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	struct tree_node *root;
	if (argc < 2) {
                fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
                exit(1);
        }
	root = get_tree_from_file(argv[1]);	/*get tree (tree_node)*/
	pid = make_proc_tree(root);	/*returns pid of root*/
	wait_for_ready_children(1);	/*wait for root process to be ready*/
	show_pstree(pid);	/* Print the process tree root at pid */
	kill(pid,SIGCONT);	/*send SIGCONT to root*/
        pid = wait(&status);	/* Wait for the root of the process tree to terminate */
        explain_wait_status(pid, status);
	return 0;
}

