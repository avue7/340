/*
 * tsh - A tiny shell program with job control
 *
 * <003978235: Athit Vue>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "globals.h"
#include "jobs.h"
#include "helper-routines.h"



/* Global variables */

char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */

/* The variable environ points to an array of pointers to string.
   This array of strings is made available to the process by the exec
   function. When the child process is created via fork(), it will 
   inherit a copy of it's parents environment. Well need this in exec.
*/
extern char **environ;


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);



/*
 * main - The shell's main routine
 */
int main(int argc, char **argv)
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    }

    exit(0); /* control never reaches here */
}

/*
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
*/
void eval(char *cmdline)
{
  /* Parse command line */
  //
  // The 'argv' vector is filled in by the parseline
  // routine below. It provides the arguments needed
  // for the execve() routine, which you'll need to
  // use below to launch a process.
  //
  char *argv[MAXARGS];

  //
  // The 'bg' variable is TRUE if the job should run
  // in background mode or FALSE if it should run in FG
  //
  int bg = parseline(cmdline, argv);
  if (argv[0] == NULL)
    return;   /* ignore empty lines */

  /* Use strcmp to compare the string pointed to by str1 and str2
     If first argument is built in command then process immediately.
     Strcmp: if ret_val is < 0 then str1 is less than str2
             if ret_val is > 0 then str1 is greater than str2
   	     if ret_val is == 0 then str1 = str2
  */
  // If builtin command (quit, jobs, fg, bg) process immediately 
  if (strcmp(argv[0], "quit") == 0)
  {
    // Invoke builtin_cmd and pass by reference (pass by alias)
    builtin_cmd(&argv[0]);
  }
  else if (strcmp(argv[0], "jobs") == 0)
  {
    builtin_cmd(&argv[0]);
  }
  else if (strcmp(argv[0],"fg") == 0)
  {
    builtin_cmd(&argv[0]);
  }
  else if (strcmp(argv[0], "bg") == 0)
  {
    builtin_cmd(&argv[0]);
  }
  else  // not a built-in cmd, fork a child and run in context of child
  {
    pid_t pid; // Declare a pid variable to store the pid
    int ret_val; // Need this to check the return of execve
    sigset_t sigset; // Create a sigset
    sigemptyset(&sigset); // Empty the sigset
    sigaddset(&sigset, SIGCHLD);  // Add SIGCHLD signal to the set
    
    // Fork and return the id of the parent and child
    pid = fork();
    
    // Child process, if pid == 0
    if (pid == 0) 
    {
      // Unblock the SIGCHLD signal that lives in the sigset
      sigprocmask(SIG_UNBLOCK, &sigset, NULL);     
      
      // Set the group id for the SIGCHILD
      // This insures that there is only one process in the fg group
      setpgid(0, 0);

      /* Executes program pointed to by filename(argv[0]). Must be a script
         or executable starting with: #!.
         Return value: none, if succeeds.
                       -1 on error and errno is set appropriately. */
      ret_val = execve(argv[0], argv, environ);
      // If execve fails then print failure and exit
      if (ret_val == -1)
      { 
        // ENOENT: file or script not exist.
        // ENOTDIR: path prefix of filename or script not in directory.
        if (errno == ENOTDIR || errno == ENOENT)
        {
          printf("%s: Command not found \n", argv[0]);
          exit(EXIT_FAILURE);  // Exit from creating child process
        }
      }       
    }
    
    // Add job to fg
    if (bg == 0)
    {
      addjob(jobs, pid, FG, cmdline);
      // Unblock the sigset(SIGCHLD)
      sigprocmask(SIG_UNBLOCK, &sigset, NULL);
      
      // Now we must wait for fg process to finish
      waitfg(pid);
    }
    else // Add job to bg
    {
      // Add the process as a bg job. Use the addjob function that lives inside
      // of jobs.h
      addjob(jobs, pid, BG, cmdline);
      // Print the added job to the command prompt
      printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
      // Unblock the sigset(SIGCHLD)      
      sigprocmask(SIG_UNBLOCK, &sigset, NULL);
    }
  }
  return;
}


/////////////////////////////////////////////////////////////////////////////
//
// builtin_cmd - If the user has typed a built-in command then execute
// it immediately. The command name would be in argv[0] and
// is a C string. We've cast this to a C++ string type to simplify
// string comparisons; however, the do_bgfg routine will need
// to use the argv array as well to look for a job number.
//
int builtin_cmd(char **argv)
{
  // Return 1 when we process is a built_in command. Return 0 otherwise.

  // Quit command: exit the command prompt when user input quit
  if (strcmp(argv[0], "quit") == 0)
  {
    exit(0);
  }
  // Jobs command: list the jobs and return 1
  else if (strcmp(argv[0], "jobs") == 0)
  {
    listjobs(jobs);
    return 1;
  }
  // Forground command: pass the argv array to do_bgfg to start a 
  //   fg process and return 1
  else if (strcmp(argv[0], "fg") == 0)
  {
    do_bgfg(argv);
    return 1;
  }
  // Background command: pass the argv array and return 1
  else if (strcmp(argv[0], "bg") == 0)
  {
    do_bgfg(argv);
    return 1;
  }
  // Return 0 when not a builtin command
  else
  {
    return 0;     /* not a builtin command */
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// do_bgfg - Execute the builtin bg and fg commands
//
void do_bgfg(char **argv)
{
  struct job_t *jobp=NULL;

  /* Ignore command if no argument */
  if (argv[1] == NULL) {
    printf("%s command requires PID or %%jobid argument\n", argv[0]);
    return;
  }

  /* Parse the required PID or %JID arg */
  if (isdigit(argv[1][0])) {
    pid_t pid = atoi(argv[1]);
    if (!(jobp = getjobpid(jobs, pid))) {
      printf("(%d): No such process\n", pid);
      return;
    }
  }
  else if (argv[1][0] == '%') {
    int jid = atoi(&argv[1][1]);
    if (!(jobp = getjobjid(jobs, jid))) {
      printf("%s: No such job\n", argv[1]);
      return;
    }
  }
  else {
    printf("%s: argument must be a PID or %%jobid\n", argv[0]);
    return;
  }

  //
  // You need to complete rest. At this point,
  // the variable 'jobp' is the job pointer
  // for the job ID specified as an argument.
  //
  // Your actions will depend on the specified command
  // so we've converted argv[0] to a string (cmd) for
  // your benefit.
  //
  
  /* For BG: if first argument is bg */
  if (strcmp(argv[0], "bg") == 0)
  {
    // If a process is stopped or has stopped in the bg
    if (jobp->state == ST)
    {
      jobp->state = BG; // Set pointer to BG
      // tell the paused job in the bg that it may resume
      kill (-jobp->pid, SIGCONT);

      // Print the command line to the command prompt
      char *print_to_cmd = (char *) &(jobp->cmdline);
      printf ("[%d] (%d) %s", jobp->jid, jobp->pid, print_to_cmd);
    }
  }
  else // for FG processes, if first argument is fg
  {
    // If the current job is in the bg or is stopped,
    //   tell the process in the fg to continue.
    if (jobp->state == BG || jobp->state == ST)
    {
      jobp->state = FG; // Set pointer to FG
     
      // Since we are running processes in the bg and want more
      //   effiency, put fg to use while processes run
      //   in the bg, we need to tell the fg that it may resume
      kill (-jobp->pid, SIGCONT);

      waitfg(jobp->pid); // Wait til fg job completes
    }
  }

  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// waitfg - Block until process pid is no longer the foreground process
//
void waitfg(pid_t pid)
{
  // Current pid is stored in struct that lives inside of jobs.h
  // Get the pid of all jobs in the fg. There should be only one 
  // in the fg at an instance. Wait until a fg job completes.
  while (pid == fgpid(jobs))
  {
    sleep(0); // sleep(0) relinquish thread to any other thread ready to run
  }
  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// Signal handlers
//


/////////////////////////////////////////////////////////////////////////////
//
// sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
//     a child job terminates (becomes a zombie), or stops because it
//     received a SIGSTOP or SIGTSTP signal. The handler reaps all
//     available zombie children, but doesn't wait for any other
//     currently running children to terminate.
//
void sigchld_handler(int sig)
{
  pid_t pid; // Declare a pid variable
  int status;
  /* waitpid: Use this function to release the zombie child of resources.
   *    This function will wait for a status changed in each of the child.
   *    If wait was not performed, then the child will remain a zombie
   *    ...for eternity...(or until comp shuts down)...mohhaahahahhaha..
   *    .must kill the zombies!!!
   * @ param -1 means wait for any child process
   * @ param &status  status 
   * @ param WNOHANG  option set: return immediately if no child has exited
   * @ param WUNTRACED option set: return if a child has stopped
   * @ return returns the pid of the child who state has changed
   *          if 0, child(ren) exist but state has not changed
   *          if -1, error
   */      
  while ((pid = waitpid (-1, &status, WNOHANG|WUNTRACED)) > 0 )
  {
    // Return true if child was stopped by delivery of signal
    if (WIFSTOPPED(status))
    {
      sig = 20;
      sigtstp_handler(20);
      getjobpid(jobs, pid)->state = ST; //set the state to ST
      int jid = pid2jid(pid); // get job id for printout
      printf ("Job [%d] (%d) stopped by signal 20\n", jid, pid);
    }
    // Return true if child was terminated via signal
    else if (WIFSIGNALED(status))
    {
      sig = 2;
      sigint_handler(2);
      int jid = pid2jid(pid);
      getjobpid(jobs, pid);
      printf("Job [%d] (%d) terminated by signal 2\n", jid, pid);
      deletejob(jobs, pid);
    }
    // Return true if child exited properly, returning to main()
    else if (WIFEXITED(status))
    {
      deletejob(jobs, pid);
    }
    else
    {
      // Do nothing
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// sigint_handler - The kernel sends a SIGINT to the shell whenver the
//    user types ctrl-c at the keyboard.  Catch it and send it along
//    to the foreground job.
//
void sigint_handler(int sig)
{
  // Get the pid job of current processor running in the foreground
  int pid = fgpid(jobs); 
  // Check to make sure that pid != 0. If it is not, that means there 
  //   is a process running in the foreground. Run the kill command to 
  //   send the SIGTSTP signal to that process 
  if (pid != 0)
  {
    kill (-pid, SIGINT);
  }
  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
//     the user types ctrl-z at the keyboard. Catch it and suspend the
//     foreground job by sending it a SIGTSTP.
//
void sigtstp_handler(int sig)
{
  // Get the pid of the foreground jobs from fgpid(jobs) function
  // store the pid to a variable
  int pid = fgpid(jobs);
  // Check to make sure that pid != 0. If it is not, that means there 
  //   is a process running in the foreground. Run the kill command to 
  //   send the SIGTSTP signal to that process 
  if (pid != 0)
  {
    kill (-pid, SIGTSTP); // Catch it here and execute the kill command. 
  }
  return;
}

/*********************
 * End signal handlers
 *********************/
