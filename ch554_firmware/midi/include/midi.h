#pragma once

//////////////////////////////////////////////////////////////////////

#define MIDI_QUEUE_LEN 16

typedef STRUCT_QUEUE(uint32, MIDI_QUEUE_LEN) midi_queue_t;
