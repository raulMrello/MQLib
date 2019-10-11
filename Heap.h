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

#if __MBED__ == 1
#include "mbed_mem_trace.h"
using namespace rtos;
#include "mdf_api_cortex.h"
#elif ESP_PLATFORM == 1
#include "sdkconfig.h"
#endif

class Heap{
public:
	/** Set debug level
	 *
	 * @param log_level debug level
	 */
	static void setDebugLevel(esp_log_level_t log_level){
        esp_log_level_set("[Heap]..........:", log_level);        
	}

	/** Allocates memory
	 *
	 * @param size Size in bytes to allocate
	 * @param dbg_trace
	 * @return pointer to the allocated memory or NULL
	 */
	static void* memAlloc(size_t size){
		if(!IS_ISR()){
			_mtx.lock();
		}
        void *ptr = malloc(size);
        if(!ptr){
            volatile int i = 0;
            while(i==0){
            }
        }
		if(!IS_ISR()){
			_mtx.unlock();
		}
        #if ESP_PLATFORM == 1
        DEBUG_TRACE_W(!IS_ISR(), "[Heap]..........:", "HEAP_8=%d, Alloc=%d", heap_caps_get_free_size(MALLOC_CAP_8BIT), size);
		#elif __MBED__==1
        mbed_stats_heap_t heap_stats;
        mbed_stats_heap_get(&heap_stats);
        DEBUG_TRACE_W(!IS_ISR(), "[Heap]..........:", "HEAP_8=%d, Alloc=%d", (heap_stats.reserved_size - heap_stats.current_size), size);
        #endif
        return ptr;
    }

	/** Releases allocated memory
	 *
	 * @param ptr Pointer to release
	 */
    static void memFree(void* ptr){
		if(!IS_ISR()){
			_mtx.lock();
		}
        #if ESP_PLATFORM == 1
    	uint32_t size = heap_caps_get_free_size(MALLOC_CAP_8BIT);

		#elif __MBED__==1
    	mbed_stats_heap_t heap_stats;
    	mbed_stats_heap_get(&heap_stats);
    	uint32_t size = heap_stats.reserved_size - heap_stats.current_size;
        #endif
        free(ptr);
        #if ESP_PLATFORM == 1
		size = heap_caps_get_free_size(MALLOC_CAP_8BIT) - size;

		#elif __MBED__==1
        mbed_stats_heap_get(&heap_stats);
        size = (heap_stats.reserved_size - heap_stats.current_size) - size;
        #endif
		if(!IS_ISR()){
			_mtx.unlock();
		}
        #if ESP_PLATFORM == 1
		DEBUG_TRACE_W(!IS_ISR(), "[Heap]..........:", "HEAP_8=%d, Free=%d", heap_caps_get_free_size(MALLOC_CAP_8BIT), size);
        #elif __MBED__==1
        DEBUG_TRACE_W(!IS_ISR(), "[Heap]..........:", "HEAP_8=%d, Free=%d", (heap_stats.reserved_size - heap_stats.current_size), size);
        #endif
    }
private:
    static Mutex _mtx;
};



#endif 
