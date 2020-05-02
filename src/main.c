#include <gtk-3.0/gtk/gtk.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <stdbool.h>
#include "inc/midi.h"

// gtk stuff
GtkWidget *g_lbl_midi_msg;
GtkWidget *g_radio_output_0;
GtkWidget *g_radio_output_1;
GtkWidget *g_chk_pass_midi_ctrl;

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

struct NoteTypeMap note_type_map[4] = {
  {
    .type = SND_SEQ_EVENT_NOTEON,
    .text = "Note On"
  },
  {
    .type = SND_SEQ_EVENT_PGMCHANGE,
    .text = "Program Change"
  },
  {
    .type = SND_SEQ_EVENT_CONTROLLER,
    .text = "Controller"
  },
  {
    .type = SND_SEQ_EVENT_NOTEOFF,
    .text = "Note Off"
  }
};

snd_seq_t *seq;
pthread_t g_alsa_midi_tid;

static int in_ports[3];
static int out_ports[3];
int out_port = 0;
int npfd;
bool pass_midi_ctrl = true;
struct pollfd *pfd;

char * note_type_text(int note_type) {
  for (int i = 0; i < 3; i++) {
    if (note_type == note_type_map[i].type)
      return note_type_map[i].text;
  }
  return note_type_map[0].text;
}

void * midi_thread(void * context_ptr)
{
  snd_seq_event_t *ev;
  char str_midi_msg[30] = {0};
  while (snd_seq_event_input(seq, &ev) >= 0) {
    //.. modify input event ..
    bool is_midi_ctrl_command = false;
    // select prev
    if (
      ev->type == note_type_map[prev_pref.msg_type].type
      && ((
        ev->type == SND_SEQ_EVENT_NOTEON
        && ev->data.note.channel == prev_pref.channel
        && ev->data.note.note == prev_pref.msg_note
      ) || (
        ev->type == SND_SEQ_EVENT_PGMCHANGE
        && ev->data.control.value == prev_pref.msg_note
      ) || (
        ev->type == SND_SEQ_EVENT_CONTROLLER
        && ev->data.control.param == (unsigned int) prev_pref.msg_note
      ))
    ) {
      is_midi_ctrl_command = true;
      out_port = 0;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_radio_output_0), TRUE);
    // select next
    } else if (
      ev->type == note_type_map[next_pref.msg_type].type
      && ((
        ev->type == SND_SEQ_EVENT_NOTEON
        && ev->data.note.channel == next_pref.channel
        && ev->data.note.note == next_pref.msg_note
      ) || (
        ev->type == SND_SEQ_EVENT_PGMCHANGE
        && ev->data.control.value == next_pref.msg_note
      )|| (
        ev->type == SND_SEQ_EVENT_CONTROLLER
        && ev->data.control.param == (unsigned int) next_pref.msg_note
      ))
    ) {
      is_midi_ctrl_command = true;
      out_port = 1;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_radio_output_1), TRUE);
    }

    switch (ev->type) {
      case SND_SEQ_EVENT_NOTEON:
        sprintf(str_midi_msg, "%s %d %d %d",
        // sprintf(str_midi_msg, "i %d o %d r %d %d | %s %d %d %d",
          // ev->source.port,
          // ev->dest.port,
          // out_port,
          // out_ports[out_port],
          note_type_text(ev->type),
          ev->data.note.channel,
          ev->data.note.note,
          ev->data.note.velocity
        );
        printf("%s\n", str_midi_msg);
        gtk_label_set_text(GTK_LABEL(g_lbl_midi_msg), str_midi_msg);
        break;
      case SND_SEQ_EVENT_PGMCHANGE:
        sprintf(str_midi_msg, "%s %d",
          note_type_text(ev->type),
          ev->data.control.value
        );
        printf("%s\n", str_midi_msg);
        gtk_label_set_text(GTK_LABEL(g_lbl_midi_msg), str_midi_msg);
        break;
      case SND_SEQ_EVENT_CONTROLLER:
        sprintf(str_midi_msg, "%s %d",
          note_type_text(ev->type),
          ev->data.control.param
        );
        printf("%s\n", str_midi_msg);
        gtk_label_set_text(GTK_LABEL(g_lbl_midi_msg), str_midi_msg);
        break;
    }

    while(gtk_events_pending()) gtk_main_iteration();

    if (is_midi_ctrl_command && !pass_midi_ctrl)
      continue;

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
  seq = midi_init_client(in_ports, 2, out_ports, 2);

  // ret = pthread_create(&g_alsa_midi_tid, NULL, midi_thread, NULL);
  pthread_create(&g_alsa_midi_tid, NULL, midi_thread, NULL);
}

void on_combobox_changed_to_int(GtkComboBox *comboBox, int *d) {
  *d = (int)gtk_combo_box_get_active(comboBox);
}

void on_spinbutton_value_changed_to_int(GtkSpinButton *spinButton, int *d) {
  *d = (int)gtk_spin_button_get_value(spinButton);
}

void on_type_change_toggle_widget(GtkComboBox *comboBox, GtkWidget *widget) {
  switch ((int)gtk_combo_box_get_active(comboBox)) {
    case 0:
    case 3:
      gtk_widget_show(widget);
      break;
    case 1:
    case 2:
      gtk_widget_hide(widget);
      break;
  }
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
    g_chk_pass_midi_ctrl = GTK_WIDGET(gtk_builder_get_object(builder, "chk_pass_midi_ctrl"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_chk_pass_midi_ctrl), pass_midi_ctrl);

    // select prev fields
    g_lbl_prev = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_prev"));
    // channel
    g_num_prev_channel = GTK_WIDGET(gtk_builder_get_object(builder, "num_prev_channel"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_num_prev_channel), prev_pref.channel);
    g_signal_connect(g_num_prev_channel, "value-changed",
      G_CALLBACK(on_spinbutton_value_changed_to_int), &prev_pref.channel);
    g_dd_prev_msg_type = GTK_WIDGET(gtk_builder_get_object(builder, "dd_prev_msg_type"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_dd_prev_msg_type), note_type_map[0].text);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_dd_prev_msg_type), note_type_map[1].text);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_dd_prev_msg_type), note_type_map[2].text);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_dd_prev_msg_type), note_type_map[3].text);
    gtk_combo_box_set_active(GTK_COMBO_BOX(g_dd_prev_msg_type), prev_pref.msg_type);
    g_signal_connect(g_dd_prev_msg_type, "changed",
      G_CALLBACK(on_combobox_changed_to_int), &prev_pref.msg_type);
    g_signal_connect(g_dd_prev_msg_type, "changed",
      G_CALLBACK(on_type_change_toggle_widget), g_num_prev_channel);
    // msg note
    g_num_prev_msg_note = GTK_WIDGET(gtk_builder_get_object(builder, "num_prev_msg_note"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_num_prev_msg_note), (double)prev_pref.msg_note);
    g_signal_connect(g_num_prev_msg_note, "value-changed",
      G_CALLBACK(on_spinbutton_value_changed_to_int), &prev_pref.msg_note);

    // select next fields
    g_lbl_next = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_next"));
    // channel
    g_num_next_channel = GTK_WIDGET(gtk_builder_get_object(builder, "num_next_channel"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_num_next_channel), next_pref.channel);
    g_signal_connect(g_num_next_channel, "value-changed",
      G_CALLBACK(on_spinbutton_value_changed_to_int), &next_pref.channel);
    g_dd_next_msg_type = GTK_WIDGET(gtk_builder_get_object(builder, "dd_next_msg_type"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_dd_next_msg_type), note_type_map[0].text);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_dd_next_msg_type), note_type_map[1].text);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_dd_next_msg_type), note_type_map[2].text);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_dd_next_msg_type), note_type_map[3].text);
    gtk_combo_box_set_active(GTK_COMBO_BOX(g_dd_next_msg_type), next_pref.msg_type);
    g_signal_connect(g_dd_next_msg_type, "changed",
      G_CALLBACK(on_combobox_changed_to_int), &next_pref.msg_type);
    g_signal_connect(g_dd_next_msg_type, "changed",
      G_CALLBACK(on_type_change_toggle_widget), g_num_next_channel);
    // msg note
    g_num_next_msg_note = GTK_WIDGET(gtk_builder_get_object(builder, "num_next_msg_note"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_num_next_msg_note), (double)next_pref.msg_note);
    g_signal_connect(g_num_next_msg_note, "value-changed",
      G_CALLBACK(on_spinbutton_value_changed_to_int), &next_pref.msg_note);

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

void on_chk_pass_midi_ctrl_toggled (GtkCheckButton *checkButton)
{
  pass_midi_ctrl = (bool)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton));
}

// called when window is closed
void on_window_main_destroy()
{
    gtk_main_quit();
}
