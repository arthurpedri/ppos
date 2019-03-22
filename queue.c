#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "queue.h"


//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila

void queue_append (queue_t **queue, queue_t *elem) {
    if(!queue){
        fprintf(stderr, "%s\n", "Fila não existe");
        return;
    }
    if(!elem){
        fprintf(stderr, "%s\n", "Elemento não existe");
        return;
    }
    if(elem->next || elem->prev){
        fprintf(stderr, "%s\n", "Elemento já está em outra fila");
        return;
    }

    if(!(*queue)){
        (*queue) = elem;
        elem->prev = elem;
        elem->next = elem;
    } else{
        elem->next = (*queue);
        elem->prev = (*queue)->prev;
        (*queue)->prev->next = elem;
        (*queue)->prev = elem;
    }
}

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: apontador para o elemento removido, ou NULL se erro

queue_t *queue_remove (queue_t **queue, queue_t *elem){
    if(!queue){
        fprintf(stderr, "%s\n", "Fila não existe");
        return NULL;
    }
    if(!elem){
        fprintf(stderr, "%s\n", "Elemento não existe");
        return NULL;
    }
    if(!(*queue)){
        fprintf(stderr, "%s\n", "Fila vazia");
        return NULL;
    }

    queue_t *first = (*queue);
    queue_t *current = first->next;

    if(first == elem){
        if(queue_size((*queue)) == 1){
            (*queue) = NULL;
        } else{
            (*queue)->prev->next = (*queue)->next;
            (*queue)->next->prev = (*queue)->prev;
            (*queue) = (*queue)->next;
        }

        elem->next = NULL;
        elem->prev = NULL;
    } else{
        while(current != first){
            if(current == elem){
                current->prev->next = current->next;
                current->next->prev = current->prev;
                current = current->next;
                elem->next = NULL;
                elem->prev = NULL;
            } else current = current->next;
        }
    }

    return elem;


}

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue){
    if(!queue){
        fprintf(stderr, "%s\n", "Fila vazia");
        return 0;
    }

    int cont = 1;

    queue_t *first = queue;
    queue_t *current = queue->next;

    while(first!=current){
        cont++;
        current = current->next;
    }

    return cont;
}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca.
//
// Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) ) {
    if(!queue){
        fprintf(stderr, "%s\n", "Fila vazia");
        return;
    }

    queue_t *first = queue;
    queue_t *current = queue->next;
    printf("%s[", name);
    print_elem(first);
    while(first!=current){
        printf(" ");
        print_elem(current);
        current = current->next;
    }
    printf("]\n" );
    return;

}
