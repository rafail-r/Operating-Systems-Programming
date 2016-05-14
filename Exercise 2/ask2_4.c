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

pid_t make_proc_tree(struct tree_node *node, int fd_father[2])
{	
	int fd1[2], fd2[2];
	int status;
	pid_t pid;
	int num1, num2;
	pid = fork();
	if (pid<0){	/*Error*/
		printf("%s :",node->name);
		perror("fork");
		exit(-1);
	}
	if (pid==0){
		change_pname(node->name);	/*Change the process name*/
		printf("%s : Created \n", node->name);	/*Message "created"*/
		if (node->nr_children==0){	/*it's a leaf*/                     
                        num1 = atoi(node->name);
                        printf("%s: %d sends %d to pipe \n", node->name,getpid(),num1);
                        if (write(fd_father[1],&num1,sizeof(num1))<0){	/*sends the number (name) to father*/
				perror("write pipe");
				exit(-1);
			}
                        sleep(SLEEP_PROC_SEC);
                        printf("%s: Exiting...\n", node->name);	/*then sleeps because we want to see the tree*/
                        exit(0);
                }		
		if (pipe(fd1)){
                	perror ("pipe");	/*create a pipe for the first child*/
                	exit(-1);
       		}
		make_proc_tree(node->children, fd1);	/*recursion*/
		if (pipe(fd2)){
                        perror ("pipe");	/*create a second pipe for the second child*/
                        exit(-1);
                }
                make_proc_tree(node->children+1, fd2);	/*recursion*/
		printf("%s: Waiting...\n", node->name);	/*else procedure is father of other procedures*/
		pid = wait(&status);            /*Waiting for children to be terminated*/
 		explain_wait_status(pid, status);
       	        pid = wait(&status);            /*Waiting for children to be terminated*/
              	explain_wait_status(pid, status);
		if (read(fd1[0],&num1,sizeof(num1)) <0){	/*read from pipe1 the value of the first child*/
                                perror("read pipe");
                                exit(-1);
                        }
		if (read(fd2[0],&num2,sizeof(num2)) <0){	/*read from pipe2 the value of the second child*/
                                perror("read pipe");
                                exit(-1);
                        }
		printf("%s : %d reads %d and %d from pipe \n" , node->name, getpid(),num1,num2);
		printf("%d : %d %s %d \n",getpid(), num1, node->name, num2);
		switch(node->name[0]){	/*choose operation*/
			case '+':
				num1 = num1+num2;
				break;
			case '-':
				num1 = num1-num2;
				break;
			case '/':
				num1 = num1/num2;
				break;
			case '*':
				num1 = num1*num2;
				break;
		}
		printf("%s: %d sends %d to pipe \n",node->name,getpid(),num1);
		if (write(fd_father[1],&num1,sizeof(num1))<0){	/*send the result to the pipe of the father*/
                                perror("write pipe");
                                exit(-1);
                        }
      		printf("%s: Exiting...\n", node->name);
        	exit(0);
	}
	return pid;
        }

int main(int argc, char *argv[])
{
	int fd[2]; /*pipe*/
        int num1;
	pid_t pid;
	int status;
	struct tree_node *root;
	if (argc != 2) {
                fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
                exit(1);
        }
	root = get_tree_from_file(argv[1]);	/*get tree (tree_node)*/
	if (pipe(fd))
    	{
      		perror ("pipe");
      		return EXIT_FAILURE;
    	}
	pid = make_proc_tree(root,fd);	/*returns pid of root (the first call of the function)*/
	sleep(SLEEP_TREE_SEC); 	/*sleep until all procedures of tree created*/
        show_pstree(pid);	/* Print the process tree root at pid */
	pid = wait(&status);    /* Wait for the root of the process tree to terminate */
        explain_wait_status(pid, status);
	if (read(fd[0],&num1,sizeof(num1)) <0 ){	/*read the result from pipe (from tree's root)*/
                                perror("read pipe");
                                exit(-1);
                        }
	printf("##################### \nThe result is : %d \n#####################\n",num1);
	return 0;
}
