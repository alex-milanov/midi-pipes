
#include <alsa/asoundlib.h>


snd_seq_t *midi_init_client(int in_ports[], int in_port_count, int out_ports[], int out_port_count)
{

    snd_seq_t *seq_handle;
    snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0);
    snd_seq_set_client_name(seq_handle, "MIDI Pipes");

    char label[20];

    for (int i = 0; i < in_port_count; i++) {
      sprintf(label, "input %d", i );
      in_ports[i] = snd_seq_create_simple_port(seq_handle, label,
        SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION);
    }

    for (int i = 0; i < out_port_count; i++) {
      sprintf(label, "output %d", i );
      out_ports[i] = snd_seq_create_simple_port(seq_handle, label,
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_APPLICATION);
    }

    return seq_handle;

}
