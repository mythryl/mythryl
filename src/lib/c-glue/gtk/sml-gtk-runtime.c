/*
 * The runtime system spawns a pthread and run the main level loop on that
 * thread. 
 *
 * NOTE: The code is slightly complex, because signal handler 
 *       can be called from everywhere, including outside of 
 *       the main event loop, or within a signal handler itself.
 *
 * Allen Leung (leunga@{cs.nyu.edu,dorsai.org})
 *
 */

/* #define DEBUG */
#define SANITY_CHECK 1

#include <gtk/gtk.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "sml-gtk-runtime.h"

#define DO(statement) do { statement } while(0)
#ifdef DEBUG 
#define LOG(msg) DO(fprintf(stderr,"%s\n",msg);)
#else
#define LOG(msg) DO()
#endif
#ifdef SANITY_CHECK
#define CHECK(call) \
DO(int rc = call; \
   if (rc) \
      smlgtk_error("%s failed at %s:%d rc=%s\n",# call,__FILE__,__LINE__,\
		   sys_errlist[rc]);\
  )
#else
#define CHECK(call) DO(call)
#endif

static void smlgtk_error(const char * fmt, ...)
{  va_list va;
   va_start(va, fmt);
   vfprintf(stderr, fmt, va); 
   va_end(va);
   exit(1);
}


/*
 * A queue for user signals
 */
typedef struct signal_queue signal_queue;
struct signal_queue
{  void * object;
   int    callbackid;
   signal_queue * prev, * next;
};

static signal_queue * first, * last;

static void signal_queue_clear()
{  signal_queue * p, * q;
   for (p = first; p; p = q)
   {  q = p->next;
      g_free(p);
   }
   first = last = NULL;
}

/*
 * A package for communicating between SML and the event loop thread.
 * All accesses to this data package should be made into critical sections.
 */
struct smlgtk_event smlgtk_event;

/*
 * The current runtime thread.
 */
static pthread_t runtime_thread;

/*
 * Is the thread running?
 */
static gboolean thread_is_running = FALSE; 

/*
 * A condition variable for communication between C and ML.
 */
typedef struct 
{  GMutex * mutex;
   GCond  * cond;
   int      on;
} condition;

static void cond_create(condition * c)
{  c->on    = 0;
   c->mutex = g_mutex_new();
   c->cond  = g_cond_new();
}

static void cond_destroy(condition * c)
{  g_assert(c->mutex != NULL);
   g_assert(c->cond  != NULL);
   g_mutex_free(c->mutex);   
   g_cond_free(c->cond);
   c->mutex = NULL; 
   c->cond  = NULL; 
}

#define cond_begin_signal(c) DO( g_mutex_lock((c).mutex); )

#define cond_end_signal(c) \
DO \
(  (c).on = 1; \
   g_cond_signal((c).cond); \
   g_mutex_unlock((c).mutex); \
)

#define cond_begin_wait(c) \
DO \
(  g_mutex_lock((c).mutex); \
   while (!(c).on) \
      g_cond_wait((c).cond, (c).mutex); \
) 

#define cond_end_wait(c) \
DO \
(  (c).on = 0; \
   g_mutex_unlock((c).mutex); \
)

#define cond_clear(c) DO( (c).on = 0; )

/*
 * Mutexes and condition variables
 */
static condition event_pending;
static condition threadOk;
static GMutex *  signal_pending;
static gboolean  signal_handler_added;
static condition response;


/*
 * Initialization. 
 * This function may be called multiple times when we are in 
 * SML's commandline.  
 */
void smlgtk_runtime_init()
{   static int init = FALSE;

    LOG("RUNTIME: smlgtk_runtime_init");

    if (*init) 
    {  /* call this only once or glib will choke */
       g_thread_init(NULL);  
       init = TRUE;
    } else 
    {  /* Clean up previous versions of these data package */
       cond_destroy(&event_pending);
       cond_destroy(&threadOk);
       cond_destroy(&response);
       g_mutex_free(signal_pending);
    }

    signal_queue_clear();

    cond_create(&event_pending);
    cond_create(&response);
    cond_create(&threadOk);

    signal_pending = g_mutex_new();
    signal_handler_added = FALSE;

    memset(&smlgtk_event,sizeof(struct smlgtk_event),0); 

    if (thread_is_running)
       CHECK(pthread_cancel(runtime_thread));

    thread_is_running = FALSE; 
}

/*
 * Clean up the state of the system
 */
void smlgtk_runtime_cleanup()
{
   LOG("RUNTIME: smlgtk_cleanup");
   pthread_cancel(runtime_thread);
   cond_destroy(&event_pending);
   cond_destroy(&threadOk);
   g_mutex_free(signal_pending);
   cond_destroy(&response);
}

static gboolean flush_signal_queue(void * data)
{   /* Drain signal queue */
    LOG("RUNTIME: flush_signal_queue");
    while (TRUE) 
    {  signal_queue * p;
       void * object = NULL;
       int    callbackid = 0;
 
       /* Get entry from event queue */ 
       LOG("RUNTIME: flush_signal_queue getting signal_pending lock");
       g_mutex_lock(signal_pending);
       p = first;
       if (p != NULL)
       {  object     = p->object;
          callbackid = p->callbackid;
          first = first->next;
          if (p == last) last = NULL;
          g_free(p);
       }
       g_mutex_unlock(signal_pending);
       LOG("RUNTIME: flush_signal_queue releasing signal_pending lock");

       /* signal_queue is empty */
       if (p == NULL) break;

       /* Send event to client */
       cond_begin_signal(event_pending);
       smlgtk_event.object     = object;
       smlgtk_event.event      = NULL;
       smlgtk_event.callbackid = callbackid;
       cond_end_signal(event_pending);

       /* Wait for it to finish */
       cond_begin_wait(response);
       cond_end_wait(response);
    }
    signal_handler_added = FALSE;
    LOG("RUNTIME: flush_signal_queue DONE");
    return FALSE;
}

/*
 * The main event "loop".  
 */
static void * smlgtk_thread(void * arg)
{
   /*
    * Start entering the event loop. 
    */
   smlgtk_event.alive = 1;
   LOG("RUNTIME: smlgtk_thread: started");
   /*cond_begin_signal(threadOk);
   cond_end_signal(threadOk);*/
   thread_is_running = TRUE; 

   /*
    * Now enter the main loop.
    */
   gdk_threads_enter();
   gtk_main();
   flush_signal_queue(NULL);
   gdk_threads_leave();

   /* When we are about to die, signal the other thread so that it knows 
    * to unblock.
    */
   cond_begin_signal(event_pending);
   smlgtk_event.alive = 0;
   cond_end_signal(event_pending);
   thread_is_running = FALSE; 
   LOG("RUNTIME: smlgtk_thread: terminated");
   return arg;
}

/*
 * This function spawns a pthread for handling Gtk events.
 */
void smlgtk_spawn_event_loop_thread() 
{   
   LOG("SML: smlgtk_spawn_event_loop_thread");
   CHECK (pthread_create(&runtime_thread, NULL, smlgtk_thread, NULL));

   /* Block until thread is ready */
   /*cond_begin_wait(threadOk);
   cond_end_wait(threadOk);*/
   LOG("SML: smlgtk_spawn_event_loop_thread: DONE");
}

void smlgtk_kill_event_loop_thread()
{
   LOG("SML: smlgtk_kill_event_loop_thread");
   CHECK(pthread_join(runtime_thread, NULL));
   LOG("SML: smlgtk_kill_event_loop_thread: DONE");
}

/*
 * A callback for handling events.
 * I'm assuming that the client cannot cause this callback to be
 * called during its processing. 
 */
gboolean smlgtk_event_callback(void * object, void *event, gpointer data)
{  gboolean reply;

   LOG("RUNTIME: smlgtk_event_callback");

   cond_begin_signal(event_pending);

   smlgtk_event.object      = object;
   smlgtk_event.event       = event;
   smlgtk_event.callbackid  = (int)data;

   cond_end_signal(event_pending);

   /* wait for client to finish */
   cond_begin_wait(response);

   reply = (gboolean)smlgtk_event.reply;

   cond_end_wait(response);
   LOG("RUNTIME: smlgtk_event_callback: DONE");
   return reply;
}

/*
 * A callback for handling signals.
 * Signals are handled differently than events because gtk signal
 * callbacks have to be reentrant!
 */
void smlgtk_signal_callback(void * object, gpointer data)
{
   LOG("RUNTIME: smlgtk_signal_callback");

   g_mutex_lock(signal_pending);
   {  signal_queue * elem = (signal_queue*)g_malloc(sizeof(signal_queue));
      elem->object     = object;
      elem->callbackid = (int)data;
      elem->next       = NULL; 
      elem->prev       = last;
      if (first == NULL) { first = elem; }
      last             = elem;
      if (!signal_handler_added)
      {  g_idle_add(flush_signal_queue,NULL); 
         signal_handler_added = TRUE;
      }
   }
   g_mutex_unlock(signal_pending);

   LOG("RUNTIME: smlgtk_signal_callback: DONE");
}


/*
 *  This is called by ML to place itself to sleep waiting
 *  for an event to occur. 
 */
int smlgtk_begin_event_wait()
{  
   int alive;
   LOG("SML: smlgtk_begin_event");
   cond_begin_wait(event_pending);
   alive = smlgtk_event.alive;
   LOG("SML: smlgtk_begin_event: DONE");
   if (alive) return 1;
   else
   { cond_end_wait(event_pending);
     return 0;
   }
}

void smlgtk_end_event_wait()
{  
   LOG("SML: smlgtk_end_event");
   cond_end_wait(event_pending);
   LOG("SML: smlgtk_end_event: DONE");
}

/*
 *  This is called by ML to write back a reply value.
 */
void smlgtk_event_reply(int reply)
{  
   LOG("SML: smlgtk_event_reply");
   cond_begin_signal(response);
   smlgtk_event.reply = reply;
   cond_end_signal(response);
   LOG("SML: smlgtk_event_reply: DONE");
}


void smlgtk_class_init(void * klass)
{
   fprintf(stderr,"%s\n","SML: smlgtk_class_init");
}

void smlgtk_object_init(void *klass, void *obj)
{
   fprintf(stderr,"%s\n","SML: smlgtk_object_init");
}
