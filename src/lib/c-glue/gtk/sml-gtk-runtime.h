#ifndef _SMLGTK_H_
#define _SMLGTK_H_

// This package contains data for
// communicating between Mythryl and C.
//
struct smlgtk_event 
{
  void * object;        /* Gtk object */
  void * event;         /* event object (NULL if it is a signal) */
  int    callbackid;    /* ml's callback id */
  int    reply;         /* reply value from ML side */
  int    alive;         /* Is the mainloop still alive? */
};


extern struct smlgtk_event smlgtk_event;

/*  
 *  A hostthread based runtime 
 */
extern void smlgtk_runtime_init();
extern void smlgtk_runtime_cleanup();

extern void smlgtk_spawn_event_loop_thread();
extern void smlgtk_kill_event_loop_thread();

extern int  smlgtk_event_callback(void * object, void * event, void * data);
extern void smlgtk_signal_callback(void * object, void * data);

extern int  smlgtk_begin_event_wait();
extern void smlgtk_end_event_wait();
extern void smlgtk_event_reply(int reply);

/*
 * Dummy methods for class initialization
 */
extern void smlgtk_class_init(void *);
extern void smlgtk_object_init(void *, void *);

#endif
