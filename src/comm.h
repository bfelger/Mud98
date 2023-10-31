////////////////////////////////////////////////////////////////////////////////
// comm.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__COMM_H
#define MUD98__COMM_H

#define MAX_HANDSHAKES 10

#include "merc.h"

#include "entities/mobile.h"
#include "entities/descriptor.h"

#include "data/damage.h"

#include "socket.h"

#include <stdint.h>
#include <stdbool.h>

typedef enum act_target_t {
    TO_ROOM = 0,
    TO_NOTVICT = 1,
    TO_VICT = 2,
    TO_CHAR = 3,
    TO_ALL = 4,
} ActTarget;

// Comm flags -- may be used on both mobs and chars
typedef enum comm_flags_t {
    COMM_QUIET              = BIT(0),
    COMM_DEAF               = BIT(1),
    COMM_NOWIZ              = BIT(2),
    COMM_NOAUCTION          = BIT(3),
    COMM_NOGOSSIP           = BIT(4),
    COMM_NOQUESTION         = BIT(5),
    COMM_NOMUSIC            = BIT(6),
    COMM_NOCLAN             = BIT(7),
    COMM_NOQUOTE            = BIT(8),
    COMM_SHOUTSOFF          = BIT(9),
    COMM_OLCX               = BIT(10),

// Display flags
    COMM_COMPACT            = BIT(11),
    COMM_BRIEF              = BIT(12),
    COMM_PROMPT             = BIT(13),
    COMM_COMBINE            = BIT(14),
    COMM_TELNET_GA          = BIT(15),
    COMM_SHOW_AFFECTS       = BIT(16),
    COMM_NOGRATS            = BIT(17),
    // Unused                 BIT(18)

// Penalties
    COMM_NOEMOTE            = BIT(19),
    COMM_NOSHOUT            = BIT(20),
    COMM_NOTELL             = BIT(21),
    COMM_NOCHANNELS         = BIT(22),
    // Unused                 BIT(23)
    COMM_SNOOP_PROOF        = BIT(24),
    COMM_AFK                = BIT(25),
} CommFlags;

typedef enum wiznet_flags_t {
    WIZ_ON                  = BIT(0),
    WIZ_TICKS               = BIT(1),
    WIZ_LOGINS              = BIT(2),
    WIZ_SITES               = BIT(3),
    WIZ_LINKS               = BIT(4),
    WIZ_DEATHS              = BIT(5),
    WIZ_RESETS              = BIT(6),
    WIZ_MOBDEATHS           = BIT(7),
    WIZ_FLAGS               = BIT(8),
    WIZ_PENALTIES           = BIT(9),
    WIZ_SACCING             = BIT(10),
    WIZ_LEVELS              = BIT(11),
    WIZ_SECURE              = BIT(12),
    WIZ_SWITCHES            = BIT(13),
    WIZ_SNOOPS              = BIT(14),
    WIZ_RESTORE             = BIT(15),
    WIZ_LOAD                = BIT(16),
    WIZ_NEWBIE              = BIT(17),
    WIZ_PREFIX              = BIT(18),
    WIZ_SPAM                = BIT(19),
} WiznetFlags;

bool can_write(Descriptor* d, PollData* poll_data);
void close_client(SockClient* client);
void close_server(SockServer* server);
bool has_new_conn(SockServer* server, PollData* poll_data);
void handle_new_connection(SockServer* server);
void init_server(SockServer* server, int port);
void nanny(Descriptor* d, char* argument);
void poll_server(SockServer* server, PollData* poll_data);
void process_client_input(SockServer* server, PollData* poll_data);
void process_client_output(PollData* poll_data, SockType type);
bool process_descriptor_output(Descriptor* d, bool fPrompt);
void read_from_buffer(Descriptor* d);
bool read_from_descriptor(Descriptor* d);
void stop_idling(Mobile* ch);
bool write_to_descriptor(Descriptor* d, char* txt, size_t length);
void show_string(Descriptor* d, char* input);
void close_socket(Descriptor* dclose);
void write_to_buffer(Descriptor* d, const char* txt, size_t length);
void send_to_char(const char* txt, Mobile* ch);
void page_to_char(const char* txt, Mobile* ch);

#define act(format, ch, arg1, arg2, type)                                      \
    act_new((format), (ch), (arg1), (arg2), (type), POS_RESTING)
void act_new(const char* format, Mobile* ch, const void* arg1,
    const void* arg2, ActTarget type, Position min_pos);

void printf_to_char(Mobile*, char*, ...);
void bugf(char*, ...);
void printf_log(char*, ...);
size_t colour(char type, Mobile* ch, char* string);
void colourconv(char* buffer, const char* txt, Mobile* ch);
void send_to_char_bw(const char* txt, Mobile* ch);
void page_to_char_bw(const char* txt, Mobile* ch);

#ifdef _MSC_VER
void PrintLastWinSockError();
#endif

#endif // MUD98__COMM_H
