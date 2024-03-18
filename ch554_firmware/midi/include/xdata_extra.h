#define MIDI_PACKET_SIZE 4

// must be a power of 2 and MIDI_QUEUE_LEN * MIDI_PACKET_SIZE must be < MAX_PACKET_SIZE (64) so
// effectively the max length is 16
#define MIDI_QUEUE_LEN 8

struct save_buffer;
struct config;
struct usb_data;

// typedef struct save_buffer save_buffer_t;
// typedef struct config config_t;

XDATA uint8 midi_send_buffer[48];
XDATA uint8 midi_recv_buffer[48];
XDATA struct save_buffer save_buffer;
XDATA uint8 queue_buffer[MIDI_QUEUE_LEN * MIDI_PACKET_SIZE];
XDATA struct config config;
