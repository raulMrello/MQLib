/*
 * MsgBroker.h
 *
 *  Created on: 15 Mar 2018
 *      Author: raulMrello
 *
 *  MsgBroker library provides an extremely simple mechanism to add publish/subscription semantics to any C/C++ project.
 *  Publish/subscribe mechanisms is formed by:
 *  - Topics: messages exchanged between software agents: publishers and subscribers.
 *  - Publishers: agents which update data topics and notify those topic updates.
 *  - Subscribers: agents which attach to topic updates and act according their new values each time the get updated.
 */

#ifndef HEAP_H
#define HEAP_H
#include "Mutex.h"
#include <stddef.h>
#include <stdlib.h>


class Heap{
public:
	static void* memAlloc(size_t size, bool dbg_trace = false){
		_mtx.lock();
        void *ptr = malloc(size);
        if(!ptr){
            volatile int i = 0;
            while(i==0){
            }
        }
        _mtx.unlock();
        DEBUG_TRACE_W((!IS_ISR() && dbg_trace), "[Heap]..........:", "HEAP_8=%d, Alloc=%d", heap_caps_get_free_size(MALLOC_CAP_8BIT), size);
        return ptr;
    }
    static void memFree(void* ptr, bool dbg_trace = false){
    	_mtx.lock();
    	uint32_t size = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        free(ptr);
        size = heap_caps_get_free_size(MALLOC_CAP_8BIT) - size;
        _mtx.unlock();
        DEBUG_TRACE_W((!IS_ISR() && dbg_trace), "[Heap]..........:", "HEAP_8=%d, Free=%d", heap_caps_get_free_size(MALLOC_CAP_8BIT), size);
    }
private:
    static Mutex _mtx;
};



#endif 
