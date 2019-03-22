#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#include "ppos.h"

#define STACKSIZE 32768		/* tamanho de pilha das threads */

task_t *CurrentTask, ContextMain;
int taskcont = 1;

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
    }
    else
    {
       perror ("Erro na criação da pilha: ") ;
       exit (1) ;
    }

    CurrentTask = &ContextMain;
    return;
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

    return task->id;

}
// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode){

    #ifdef DEBUG
	printf ("task_exit: tarefa %d sendo encerrada\n", CurrentTask->id) ;
	#endif

    task_switch(&ContextMain);
    return;
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task){
    task_t *aux = CurrentTask;
    CurrentTask = task;

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
