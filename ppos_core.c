#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>


#include "ppos.h"
//#define DEBUG 1
#define QUANTUM 20
#define STACKSIZE 32768		/* tamanho de pilha das threads */

task_t *CurrentTask, ContextMain, *readyQueue, Dispatcher;
int taskcont = 1, userTasks = 0;
unsigned int clockticks;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

// estrutura de inicialização to timer
struct itimerval timer;

// tratador do sinal
void tratador (int signum)
{
    clockticks++;

    if (CurrentTask->usertask){
        CurrentTask->ticks--;
    }

    if (CurrentTask->ticks<=0){
        CurrentTask->ticks = QUANTUM;
        task_yield();
    }
}

unsigned int systime(){
    return clockticks;
}

task_t *scheduler (){
    if (readyQueue){
        task_t *maior, *current;
        maior = readyQueue;
        maior->age = maior->age - 1;
        current = readyQueue->next;
        while (current != readyQueue){
            current->age = current->age - 1;
            if (current->age < maior->age)
                maior = current;
            current = current->next;
        }
        #ifdef DEBUG
		printf ("Maior Prioridade: %d, id: %d ", maior->age, maior->id) ;
		#endif

        task_setprio(maior, maior->prio);

        return maior;

    }
    return NULL;
}

void dispatcher_body () // dispatcher é uma tarefa
{
    task_t *next;
    unsigned int init_time = 0;
    unsigned int final_time = 0;

    while ( userTasks > 0 )
    {


        next = scheduler() ;  // scheduler é uma função
        #ifdef DEBUG
        printf ("Dispatcher lançando tarefa %d ", next->id) ;
		#endif
        if (next)
        {
          // ações antes de lançar a tarefa "next", se houverem
            init_time = systime();
            task_switch (next) ; // transfere controle para a tarefa "next"
         // ações após retornar da tarefa "next", se houverem
            final_time = systime();
            next->proc_time = next->proc_time + (final_time - init_time);

            init_time = 0, final_time = 0;
            if (next->exit != -1){
                free(next->stack);
                queue_remove((queue_t**) &readyQueue, (queue_t*) next);
            }
        }
    }

    task_exit(0) ; // encerra a tarefa dispatcher
}

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0) ;
    char *stack ;

    getcontext (&ContextMain.context) ;

    stack = malloc (STACKSIZE) ;
    if (stack)
    {
       ContextMain.stack = stack ;
       ContextMain.prev = ContextMain.next = NULL ;
       ContextMain.context.uc_stack.ss_sp = stack ;
       ContextMain.context.uc_stack.ss_size = STACKSIZE ;
       ContextMain.context.uc_stack.ss_flags = 0 ;
       ContextMain.context.uc_link = 0 ;
       ContextMain.id = 0 ;
       ContextMain.prio = 0;
       ContextMain.age = 0;
       ContextMain.exit = -1;
       ContextMain.usertask = 1;
       ContextMain.ticks = QUANTUM;
       ContextMain.exe_time = 0;
       ContextMain.proc_time = 0;
       ContextMain.activations = 0;
    }
    else
    {
       perror ("Erro na criação da pilha: ") ;
       exit (1) ;
    }

    //Cria o tratador para os sinais do temporizador
    action.sa_handler = tratador ;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        perror ("Erro em sigaction: ") ;
        exit (1) ;
    }

    //INICIALIZA TEMPORIZADOR
    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000 ;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0 ;      // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000 ;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0 ;   // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Erro em setitimer: ") ;
        exit (1) ;
    }

    clockticks = 0;


    CurrentTask = &ContextMain;

	task_create(&Dispatcher, dispatcher_body, "");

    return;
}


// operações de escalonamento ==================================================

// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield (){
    if (CurrentTask->id == 1){
        task_switch(&ContextMain);
    } else {
        task_switch(&Dispatcher);
    }

    return;
}

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio){
    if (prio > 20){
        prio = 20;
    } else if (prio < -20){
        prio = -20;
    }

    if (!task){
        CurrentTask->prio = prio;
        CurrentTask->age = prio;
    } else {
        task->prio = prio;
        task->age = prio;
    }

    return;
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task){
    if(!task){
        return CurrentTask->prio;
    }
    return task->prio;
}

// gerência de tarefas =========================================================

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task,			// descritor da nova tarefa
                 void (*start_func)(void *),	// funcao corpo da tarefa
                 void *arg)// argumentos para a tarefa
{
    char *stack ;

    getcontext (&task->context) ;

    stack = malloc (STACKSIZE) ;
    if (stack)
    {
       task->stack = stack ;
       task->prev = task->next = NULL ;
       task->id = taskcont ;
       task->context.uc_stack.ss_sp = stack ;
       task->context.uc_stack.ss_size = STACKSIZE ;
       task->context.uc_stack.ss_flags = 0 ;
       task->context.uc_link = 0 ;
       task->prio = 0;
       task->age = 0;
       task->exit = -1;
       task->usertask = 0;
       task->ticks = QUANTUM;
       task->exe_time = 0;
       task->proc_time = 0;
       task->activations = 0;
    }
    else
    {
       perror ("Erro na criação da pilha: ") ;
       exit (1) ;
    }

    taskcont++;

    makecontext (&task->context, (void*)(*start_func), 1, arg) ;

    #ifdef DEBUG
    printf ("task_create: criou tarefa %d\n", task->id) ;
    #endif

    if (task->id != 1){ //not dispatcher
        queue_append((queue_t**) &readyQueue, (queue_t*) task);
        task->usertask = 1;
        #ifdef DEBUG
		printf ("Tarefa %d inserida na fila de prontas\n", task->id) ;
		#endif
        userTasks++;
    }

    return task->id;

}
// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode){

    #ifdef DEBUG
	printf ("task_exit: tarefa %d sendo encerrada\n", CurrentTask->id) ;
	#endif

    CurrentTask->exit = exitCode;
    CurrentTask->exe_time = systime();
	printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", CurrentTask->id, CurrentTask->exe_time, CurrentTask->proc_time, CurrentTask->activations);


    if(CurrentTask->id == 1){
        free(CurrentTask->stack);
        task_switch(&ContextMain);
    } else {
        userTasks--;
        task_switch(&Dispatcher);
    }

    return;
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task){
    task_t *aux = CurrentTask;
    CurrentTask = task;
    task->activations++;
    #ifdef DEBUG
	printf ("task_switch: trocando contexto %d -> %d\n", aux->id, task->id) ;
	#endif

    swapcontext(&aux->context, &task->context);
    return 0;
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id (){
    return (CurrentTask->id);
}
