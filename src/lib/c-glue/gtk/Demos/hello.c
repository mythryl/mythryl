/* example-start helloworld2 helloworld2.c */

#include <gtk/gtk.h>

/* Our new improved callback.  The data passed to this function
 *  * is printed to stdout. */
void callback( GtkWidget *widget, gpointer data)
{
    g_print ("Hello again - %s was pressed\n", (char *) data);
}

/* another callback */
gint delete_event( GtkWidget *widget, GdkEvent  *event, gpointer   data )
{
    gtk_main_quit(); 
    return(FALSE);
}

int main( int   argc, char *argv[] )
{
    /* GtkWidget is the storage type for widgets */
    GtkWidget *window;
    GtkWidget *button;
    GtkWidget *box1;

    gtk_init (&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    gtk_window_set_title (GTK_WINDOW (window), "Hello Buttons!");

    gtk_signal_connect (GTK_OBJECT (window), "delete_event", GTK_SIGNAL_FUNC (delete_event), NULL); 
     gtk_container_set_border_width (GTK_CONTAINER (window), 10);

     box1 = gtk_hbox_new(FALSE, 0);
     gtk_container_add (GTK_CONTAINER (window), box1);

     button = gtk_button_new_with_label ("Button 1");
     gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (callback), (gpointer) "button 1");

     gtk_box_pack_start(GTK_BOX(box1), button, TRUE, TRUE, 0);
     gtk_widget_show(button);

     button = gtk_button_new_with_label ("Button 2");
     gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (callback), (gpointer) "button 2");
     gtk_box_pack_start(GTK_BOX(box1), button, TRUE, TRUE, 0);

     gtk_widget_show(button);
     gtk_widget_show(box1);
     gtk_widget_show (window);
										    gtk_main ();

									        return(0);
}
/* example-end */


