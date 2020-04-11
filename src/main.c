#include <gtk/gtk.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include "inc/midi.h"

GtkWidget *g_lbl_midi_msg;
GtkWidget *g_radio_output_0;
GtkWidget *g_radio_output_1;

snd_seq_t *seq;
pthread_t g_alsa_midi_tid;

static int in_ports[3];
static int out_ports[3];
int out_port = 0;
int npfd;
struct pollfd *pfd;

void * midi_thread(void * context_ptr)
{
  snd_seq_event_t *ev;
  char str_midi_msg[30] = {0};
  while (snd_seq_event_input(seq, &ev) >= 0) {
    //.. modify input event ..
    if (ev->type == SND_SEQ_EVENT_NOTEON) {
      if (ev->data.note.channel == 0 && ev->data.note.note == 14) {
        out_port = 0;
        // gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_radio_output_1), FALSE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_radio_output_0), TRUE);
      } else if (ev->data.note.channel == 0 && ev->data.note.note == 15) {
        out_port = 1;
        // gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_radio_output_0), FALSE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_radio_output_1), TRUE);
      }

      sprintf(str_midi_msg, "NoteON %d %d %d",
        ev->data.note.channel,
        ev->data.note.note,
        ev->data.note.velocity
      );
      printf("%s\n", str_midi_msg);


      gtk_label_set_text(GTK_LABEL(g_lbl_midi_msg), str_midi_msg);

      while(gtk_events_pending()) gtk_main_iteration();

    }

    snd_seq_ev_set_source(ev, out_ports[out_port]);
    snd_seq_ev_set_subs( ev );
    snd_seq_ev_set_direct( ev );
    snd_seq_event_output_direct(seq, ev );
    snd_seq_free_event(ev);

  }
  return NULL;
}

void midi_init(void)
{
  // int ret;
  seq = midi_init_client(in_ports, 1, out_ports, 2);

  // ret = pthread_create(&g_alsa_midi_tid, NULL, midi_thread, NULL);
  pthread_create(&g_alsa_midi_tid, NULL, midi_thread, NULL);
}


int main(int argc, char *argv[])
{
    GtkBuilder      *builder;
    GtkWidget       *window;

    gtk_init(&argc, &argv);

    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, "window_main.glade", NULL);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));
    gtk_builder_connect_signals(builder, NULL);

    g_lbl_midi_msg = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_midi_msg"));
    g_radio_output_0 = GTK_WIDGET(gtk_builder_get_object(builder, "radio_output_0"));
    g_radio_output_1 = GTK_WIDGET(gtk_builder_get_object(builder, "radio_output_1"));

    g_object_unref(builder);

    gtk_widget_show(window);
    gtk_main();

    return 0;
}


void on_midion_clicked()
{
  midi_init();
}

void on_radio_output_0_toggled (GtkToggleButton *togglebutton, GtkLabel *label)
{
    out_port = 0;
    // if (gtk_toggle_button_get_active(togglebutton)) out_port = 0;
}

void on_radio_output_1_toggled (GtkToggleButton *togglebutton, GtkLabel *label)
{
    out_port = 1;
    // if (gtk_toggle_button_get_active(togglebutton)) out_port = 1;
}

// called when window is closed
void on_window_main_destroy()
{
    gtk_main_quit();
}
