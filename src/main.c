#include <gtk/gtk.h>
#include <alsa/asoundlib.h>
#include "inc/midi.h"

GtkWidget *g_lbl_hello;
GtkWidget *g_lbl_count;

static snd_seq_t *seq;
static int in_ports[3];
static int out_ports[3];
int out_port = 0;

void midi_open(void)
{
    snd_seq_event_t *ev;

    seq = midi_init_client(in_ports, 1, out_ports, 2);

    // printf("the number is %d \n", vectorArray.at(place) );
    while (snd_seq_event_input(seq, &ev) >= 0) {
      //.. modify input event ..
      if (ev->type == SND_SEQ_EVENT_NOTEON) {
        if (ev->data.note.channel == 0 && ev->data.note.note == 14) {
          out_port = 0;
        } else if (ev->data.note.channel == 0 && ev->data.note.note == 15) {
          out_port = 1;
        }
        printf("NoteON %d %d %d \n",
          ev->data.note.channel,
          ev->data.note.note,
          ev->data.note.velocity
        );

      }

      snd_seq_ev_set_source(ev, out_ports[out_port]);
      snd_seq_ev_set_subs( ev );
      snd_seq_ev_set_direct( ev );
      snd_seq_event_output_direct(seq, ev );
      snd_seq_free_event(ev);

    }


}
snd_seq_event_t *midi_read(snd_seq_t *seq)
{
    snd_seq_event_t *ev = NULL;
    snd_seq_event_input(seq, &ev);
    return ev;
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

    g_lbl_hello = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_hello"));
    g_lbl_count = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_count"));

    g_object_unref(builder);

    gtk_widget_show(window);
    gtk_main();

    return 0;
}

void on_midion_clicked()
{
  midi_open();
}

// called when button is clicked
void on_btn_hello_clicked()
{
    static unsigned int count = 0;
    char str_count[30] = {0};

    gtk_label_set_text(GTK_LABEL(g_lbl_hello), "Hello, world!");
    count++;
    sprintf(str_count, "%d", count);
    gtk_label_set_text(GTK_LABEL(g_lbl_count), str_count);
}

// called when window is closed
void on_window_main_destroy()
{
    gtk_main_quit();
}
