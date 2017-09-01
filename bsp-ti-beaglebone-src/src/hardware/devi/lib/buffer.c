/*
 * Copyright 2003, QNX Software Systems Ltd. All Rights Reserved.
 *
 * This source code may contain confidential information of QNX Software
 * Systems Ltd.  (QSSL) and its licensors. Any use, reproduction,
 * modification, disclosure, distribution or transfer of this software,
 * or any software which includes or is based upon any of this code, is
 * prohibited unless expressly authorized by QSSL by written agreement. For
 * more information (including whether this source code file has been
 * published) please email licensing@qnx.com.
 */



/*
 * buffer.c
 *
 * Buffer manipulation functions.
 *
 */
#include <sys/devi.h>

#define	MAX_BUFF_BYTES	512


buffer_t *
buff_create(unsigned size, unsigned rsize)
{
        buffer_t *cbptr;

        if(cbptr = smalloc(sizeof *cbptr)) {
    
                cbptr->bufsize = size ? size : 
                        MAX_BUFF_BYTES - (MAX_BUFF_BYTES % rsize);
                cbptr->cnt = 0;
    
                if(cbptr->head = cbptr->tail = cbptr->buff = 
                   malloc(cbptr->bufsize)) {

                        pthread_mutex_init(&cbptr->mutex, NULL);

                } else {
                        free(cbptr);
                        cbptr = NULL;
                }
        }
        
        return (cbptr);
}


int
buff_append(buffer_t *bptr, char *dptr, int n) 
{
        int		t;
  
        if(n && (bptr->cnt < bptr->bufsize)) {

                while(n) {

                        t = min(n, (&bptr->buff[bptr->bufsize] - bptr->head));
                        if(t) {

                                if(dptr) {
                                        memcpy(bptr->head, dptr, t);
                                        dptr += t;
                                }

                                bptr->cnt += t;
                                bptr->head += t;

                                if(bptr->head >= &bptr->buff[bptr->bufsize]) {
                                        bptr->head = bptr->buff;
                                }
                                n -= t;
                        }
                }
        }
        
        return (bptr->cnt);
}


int 
buff_delete(buffer_t *bptr, char *dptr, int n) 
{
        int					t, v = 0;

        pthread_mutex_lock(&bptr->mutex);

        if(bptr->cnt) {
                
                while(n) {

                        t = min(n, bptr->cnt);

                        if(t > 0) {
                                if(dptr) {
                                        memcpy(dptr, bptr->tail, t);
                                        dptr += t;
                                }

                                bptr->tail += t;
                                bptr->cnt -= t;
                                if(bptr->tail >= &bptr->buff[bptr->bufsize]) {
                                        bptr->tail = bptr->buff;
                                }

                                n -= t;
                                v += t;
                        } else {
                                n = 0;
                        }
                }
        }
        
        pthread_mutex_unlock(&bptr->mutex);
        
        return (v);
}


int
buff_flush(buffer_t *bptr)
{
        bptr->tail = bptr->head = bptr->buff;
        bptr->cnt = 0;
  
        return (EOK);
}


int
buff_putc(buffer_t *bptr, char c) 
{

        static intrspin_t   spin;
        InterruptLock(&spin);

        if(bptr->cnt < bptr->bufsize) {
                bptr->cnt++;
                *bptr->head++ = c;
                if(bptr->head >= &bptr->buff[bptr->bufsize]) {
                        bptr->head = bptr->buff;
                }
        } 
        
        InterruptUnlock(&spin);
        
        return(bptr->cnt);
}


int
buff_getc(buffer_t *bptr, char * c) 
{
        
        static intrspin_t   spin;
        InterruptLock(&spin);
        
        if(bptr->cnt == 0) {
                
                InterruptUnlock(&spin);
                
                return (-1);
        }
  
        *c = *bptr->tail;
        --bptr->cnt;
        
        if(++bptr->tail >= &bptr->buff[bptr->bufsize]) {
                bptr->tail = bptr->buff;
        }
        
        InterruptUnlock(&spin);
        
        return 0;
}


int
buff_waiting(buffer_t *bptr) 
{
        return (bptr->cnt);
}





