#include <gtk-3.0/gtk/gtk.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include "inc/midi.h"

// gtk stuff
GtkWidget *g_lbl_midi_msg;
GtkWidget *g_radio_output_0;
GtkWidget *g_radio_output_1;

// select prev fields
GtkWidget *g_lbl_prev;
GtkWidget *g_num_prev_channel;
GtkWidget *g_dd_prev_msg_type;
GtkWidget *g_num_prev_msg_note;
// select next fields
GtkWidget *g_lbl_next;
GtkWidget *g_num_next_channel;
GtkWidget *g_dd_next_msg_type;
GtkWidget *g_num_next_msg_note;

struct CtrlPref {
  int channel;
  int msg_type;
  int msg_note;
};

struct CtrlPref prev_pref = {
  .channel = 0,
  .msg_type = 0,
  .msg_note = 14,
};

struct CtrlPref next_pref = {
  .channel = 0,
  .msg_type = 0,
  .msg_note = 15,
};

struct NoteTypeMap {
  int type;
  char text[20];
};

struct NoteTypeMap note_type_map[3] = {
  {
    .type = SND_SEQ_EVENT_NOTEON,
    .text = "Note On"
  },
  {
    .type = SND_SEQ_EVENT_NOTEOFF,
    .text = "Note Off"
  },
  {
    .type = SND_SEQ_EVENT_PGMCHANGE,
    .text = "Program Change"
  }
};

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

    // select prev
    if (
      ev->type == note_type_map[prev_pref.msg_type].type
      && ev->data.note.channel == prev_pref.channel
      && ev->data.note.note == prev_pref.msg_note
    ) {
      out_port = 0;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_radio_output_0), TRUE);
    // select next
    } else if (
      ev->type == note_type_map[next_pref.msg_type].type
      && ev->data.note.channel == next_pref.channel
      && ev->data.note.note == next_pref.msg_note
    ) {
      out_port = 1;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_radio_output_1), TRUE);
    }

    if (ev->type == SND_SEQ_EVENT_NOTEON) {

      sprintf(str_midi_msg, "NoteON %d %d %d",
        ev->data.note.channel,
        ev->data.note.note,
        ev->data.note.velocity
      );
      printf("%s\n", str_midi_msg);
      printf("note_on enum %d\n", SND_SEQ_EVENT_NOTEON);
      printf("note_off enum %d\n", SND_SEQ_EVENT_NOTEOFF);
      printf("pgm_change enum %d\n", SND_SEQ_EVENT_PGMCHANGE);

      gtk_label_set_text(GTK_LABEL(g_lbl_midi_msg), str_midi_msg);

    }

    while(gtk_events_pending()) gtk_main_iteration();

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

    // select prev fields
    g_lbl_prev = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_prev"));
    g_num_prev_channel = GTK_WIDGET(gtk_builder_get_object(builder, "num_prev_channel"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_num_prev_channel), prev_pref.channel);
    g_dd_prev_msg_type = GTK_WIDGET(gtk_builder_get_object(builder, "dd_prev_msg_type"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_dd_prev_msg_type), note_type_map[0].text);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_dd_prev_msg_type), note_type_map[1].text);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_dd_prev_msg_type), note_type_map[2].text);
    gtk_combo_box_set_active(GTK_COMBO_BOX(g_dd_prev_msg_type), prev_pref.msg_type);
    g_num_prev_msg_note = GTK_WIDGET(gtk_builder_get_object(builder, "num_prev_msg_note"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_num_prev_msg_note), (double)prev_pref.msg_note);

    // select next fields
    g_lbl_next = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_next"));
    g_num_next_channel = GTK_WIDGET(gtk_builder_get_object(builder, "num_next_channel"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_num_next_channel), next_pref.channel);
    g_dd_next_msg_type = GTK_WIDGET(gtk_builder_get_object(builder, "dd_next_msg_type"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_dd_next_msg_type), note_type_map[0].text);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_dd_next_msg_type), note_type_map[1].text);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_dd_next_msg_type), note_type_map[2].text);
    gtk_combo_box_set_active(GTK_COMBO_BOX(g_dd_next_msg_type), next_pref.msg_type);
    g_num_next_msg_note = GTK_WIDGET(gtk_builder_get_object(builder, "num_next_msg_note"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_num_next_msg_note), (double)next_pref.msg_note);

    g_object_unref(builder);

    gtk_widget_show(window);
    gtk_main();

    // midi_init();

    return 0;
}


void on_window_main_show()
{
  midi_init();
}

// select prev
void on_num_prev_msg_note_value_changed(GtkSpinButton *spinButton,  gpointer d) {
  prev_pref.msg_note = (int)gtk_spin_button_get_value(spinButton);
  // printf("spin-button1-vlaue: %f\n", gtk_spin_button_get_value(spinButton));
}
// select next
void on_num_next_msg_note_value_changed(GtkSpinButton *spinButton,  gpointer d) {
  next_pref.msg_note = (int)gtk_spin_button_get_value(spinButton);
  // printf("spin-button2-vlaue: %f\n", gtk_spin_button_get_value(spinButton));
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
