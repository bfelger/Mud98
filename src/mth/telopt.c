/***************************************************************************
 * Mud Telopt Handler 1.5 by Igor van den Hoven                  2009-2019 *
 ***************************************************************************/

#include "merc.h"

#include "mth.h"

#include "db.h"
#include "comm.h"
#include "config.h"
#include "telnet.h"

#include "data/class.h"
#include "data/race.h"
#include "data/skill.h"

#include "entities/mobile.h"
#include "entities/player_data.h"

#include <stdbool.h>
#include <stdint.h>

void debug_telopts(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_do_eor(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_will_ttype(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_sb_ttype_is(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_sb_naws(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_will_new_environ(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_sb_new_environ(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_do_charset(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_sb_charset(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_do_mssp(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_do_msdp(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_sb_msdp(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_do_gmcp(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_sb_gmcp(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_do_mccp2(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_dont_mccp2(Descriptor* d, unsigned char* src, size_t srclen);
size_t skip_sb(Descriptor* d, unsigned char* src, size_t srclen);
	 
void end_mccp2(Descriptor* d);
	 
bool start_mccp2(Descriptor* d);
void process_mccp2(Descriptor* d);
	 
void end_mccp3(Descriptor* d);
size_t process_do_mccp3(Descriptor* d, unsigned char* src, size_t srclen);
size_t process_sb_mccp3(Descriptor* d, unsigned char* src, size_t srclen);
	 
void send_eor(Descriptor* d);

void descriptor_printf(Descriptor* d, char* fmt, ...);

struct telopt_type {
	size_t size;
	unsigned char* code;
	uintptr_t func;
};

typedef size_t TeloptFunc(Descriptor*, unsigned char*, size_t);

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const struct telopt_type telopt_table[] = {
	{ 3, (unsigned char[]) { IAC, DO,   TELOPT_EOR, 0 },						U(process_do_eor)			},
	{ 3, (unsigned char[]) { IAC, WILL, TELOPT_TTYPE, 0 },						U(process_will_ttype)		},
	{ 4, (unsigned char[]) { IAC, SB,   TELOPT_TTYPE, ENV_IS, 0 },				U(process_sb_ttype_is)		},
	{ 3, (unsigned char[]) { IAC, SB,   TELOPT_NAWS, 0 },						U(process_sb_naws)			},
	{ 3, (unsigned char[]) { IAC, WILL, TELOPT_NEW_ENVIRON, 0 },				U(process_will_new_environ)	},
	{ 3, (unsigned char[]) { IAC, SB,   TELOPT_NEW_ENVIRON, 0 },				U(process_sb_new_environ)	},
	{ 3, (unsigned char[]) { IAC, DO,   TELOPT_CHARSET, 0 },					U(process_do_charset)		},
	{ 3, (unsigned char[]) { IAC, SB,   TELOPT_CHARSET, 0 },					U(process_sb_charset)		},
	{ 3, (unsigned char[]) { IAC, DO,   TELOPT_MSSP, 0 },						U(process_do_mssp)			},
	{ 3, (unsigned char[]) { IAC, DO,   TELOPT_MSDP, 0 },						U(process_do_msdp)			},
	{ 3, (unsigned char[]) { IAC, SB,   TELOPT_MSDP, 0 },						U(process_sb_msdp)			},
	{ 3, (unsigned char[]) { IAC, DO,   TELOPT_GMCP, 0 },						U(process_do_gmcp)			},
	{ 3, (unsigned char[]) { IAC, SB,   TELOPT_GMCP, 0 },						U(process_sb_gmcp)			},
	{ 3, (unsigned char[]) { IAC, DO,   TELOPT_MCCP2, 0 },						U(process_do_mccp2)			},
	{ 3, (unsigned char[]) { IAC, DONT, TELOPT_MCCP2, 0 },						U(process_dont_mccp2)		},
	{ 3, (unsigned char[]) { IAC, DO,   TELOPT_MCCP3, 0 },						U(process_do_mccp3)			},
	{ 5, (unsigned char[]) { IAC, SB,   TELOPT_MCCP3, IAC, SE, 0 },				U(process_sb_mccp3)			},
	{ 0, NULL,																	0							}
};

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif

/*
	Call this to announce support for telopts marked as such in tables.c
*/

void announce_support(Descriptor* d)
{
	int i;

	for (i = 0; i < 255; i++)
	{
		if (telnet_table[i].flags)
		{
			if (HAS_BIT(telnet_table[i].flags, ANNOUNCE_WILL))
			{
				descriptor_printf(d, "%c%c%c", IAC, WILL, i);
			}
			if (HAS_BIT(telnet_table[i].flags, ANNOUNCE_DO))
			{
				descriptor_printf(d, "%c%c%c", IAC, DO, i);
			}
		}
	}
}

/*
	Call this right before a copyover to reset the telnet state
*/

void unannounce_support(Descriptor* d)
{
	int i;

	end_mccp2(d);
	end_mccp3(d);

	for (i = 0; i < 255; i++)
	{
		if (telnet_table[i].flags)
		{
			if (HAS_BIT(telnet_table[i].flags, ANNOUNCE_WILL))
			{
				descriptor_printf(d, "%c%c%c", IAC, WONT, i);
			}
			if (HAS_BIT(telnet_table[i].flags, ANNOUNCE_DO))
			{
				descriptor_printf(d, "%c%c%c", IAC, DONT, i);
			}
		}
	}
}

/*
	This is the main routine that strips out and handles telopt negotiations.
	It also deals with \r and \0 so commands are separated by a single \n.
*/

size_t translate_telopts(Descriptor* d, unsigned char* src, size_t srclen, unsigned char* out, size_t outlen)
{
	size_t cnt, skip;
	unsigned char* pti, * pto;

	pti = src;
	pto = out + outlen;

#ifndef NO_ZLIB

	if (srclen > 0 && d->mth->mccp3)
	{
		d->mth->mccp3->next_in = pti;
		d->mth->mccp3->avail_in = (uInt)srclen;

		d->mth->mccp3->next_out = mud->mccp_buf;
		d->mth->mccp3->avail_out = (uInt)mud->mccp_len;

	inflate:

		switch (inflate(d->mth->mccp3, Z_SYNC_FLUSH))
		{
		case Z_BUF_ERROR:
			if (d->mth->mccp3->avail_out == 0)
			{
				unsigned char* temp = (unsigned char*)alloc_mem(mud->mccp_len * 2);
				memcpy(temp, mud->mccp_buf, mud->mccp_len);
				free_mem(mud->mccp_buf, mud->mccp_len);
				mud->mccp_len *= 2;
				mud->mccp_buf = temp;

				d->mth->mccp3->avail_out = (uInt)(mud->mccp_len / 2);
				d->mth->mccp3->next_out = mud->mccp_buf + mud->mccp_len / 2;

				goto inflate;
			}
			else
			{
				descriptor_printf(d, "%c%c%c", IAC, DONT, TELOPT_MCCP3);
				inflateEnd(d->mth->mccp3);
				free_mem(d->mth->mccp3, sizeof(z_stream));
				d->mth->mccp3 = NULL;
				srclen = 0;
			}
			break;

		case Z_OK:
			if (d->mth->mccp3->avail_out == 0)
			{
				unsigned char* temp = (unsigned char*)alloc_mem(mud->mccp_len * 2);
				memcpy(temp, mud->mccp_buf, mud->mccp_len);
				free_mem(mud->mccp_buf, mud->mccp_len);
				mud->mccp_len *= 2;
				mud->mccp_buf = temp;

				d->mth->mccp3->avail_out = (uInt)(mud->mccp_len / 2);
				d->mth->mccp3->next_out = mud->mccp_buf + mud->mccp_len / 2;

				goto inflate;
			}
			srclen = d->mth->mccp3->next_out - mud->mccp_buf;
			pti = mud->mccp_buf;

			if (srclen + outlen > INPUT_BUFFER_SIZE)
			{
				srclen = INPUT_BUFFER_SIZE - outlen - 1;
			}
			break;

		case Z_STREAM_END:
			log_descriptor_printf(d, "MCCP3: Compression end, disabling MCCP3.");

			skip = d->mth->mccp3->next_out - mud->mccp_buf;

			pti += (srclen - d->mth->mccp3->avail_in);
			srclen = d->mth->mccp3->avail_in;

			inflateEnd(d->mth->mccp3);
			free_mem(d->mth->mccp3, sizeof(z_stream));
			d->mth->mccp3 = NULL;

			while (skip + srclen + 1 > mud->mccp_len)
			{
				unsigned char* temp = (unsigned char*)alloc_mem(mud->mccp_len * 2);
				memcpy(temp, mud->mccp_buf, mud->mccp_len);
				free_mem(mud->mccp_buf, mud->mccp_len);
				mud->mccp_len *= 2;
				mud->mccp_buf = temp;
			}
			memcpy(mud->mccp_buf + skip, pti, srclen);
			pti = mud->mccp_buf;
			srclen += skip;
			break;

		default:
			log_descriptor_printf(d, "MCCP3: Compression error, disabling MCCP3.");
			descriptor_printf(d, "%c%c%c", IAC, DONT, TELOPT_MCCP3);
			inflateEnd(d->mth->mccp3);
			free_mem(d->mth->mccp3, sizeof(z_stream));
			d->mth->mccp3 = NULL;
			srclen = 0;
			break;
		}
	}

#endif

	// packet patching

	if (d->mth->teltop)
	{
		if (d->mth->teltop + srclen + 1 < INPUT_BUFFER_SIZE)
		{
			memcpy(d->mth->telbuf + d->mth->teltop, pti, srclen);

			srclen += d->mth->teltop;

			pti = (unsigned char*)d->mth->telbuf;
		}
		else
		{
			// You can close the socket here for input spamming
		}
		d->mth->teltop = 0;
	}

	while (srclen > 0)
	{
		switch (*pti)
		{
		case IAC:
			skip = 2;

			debug_telopts(d, pti, srclen);

			for (cnt = 0; telopt_table[cnt].code; cnt++)
			{
				if (srclen < telopt_table[cnt].size)
				{
					if (!memcmp(pti, telopt_table[cnt].code, srclen))
					{
						skip = telopt_table[cnt].size;

						break;
					}
				}
				else
				{
					if (!memcmp(pti, telopt_table[cnt].code, telopt_table[cnt].size)) {
						TeloptFunc* func = (TeloptFunc*)telopt_table[cnt].func;
						skip = (*func)(d, pti, srclen);

						if (telopt_table[cnt].func == (uintptr_t)process_sb_mccp3)
						{
							return translate_telopts(d, pti + skip, srclen - skip, out, pto - out);
						}
						break;
					}
				}
			}

			if (telopt_table[cnt].code == NULL && srclen > 1)
			{
				switch (pti[1])
				{
				case WILL:
				case DO:
				case WONT:
				case DONT:
					skip = 3;
					break;

				case SB:
					skip = skip_sb(d, pti, srclen);
					break;

				case IAC:
					*pto++ = *pti++;
					srclen--;
					skip = 1;
					break;

				default:
					if (TELCMD_OK(pti[1]))
					{
						skip = 2;
					}
					else
					{
						skip = 1;
					}
					break;
				}
			}

			if (skip <= srclen)
			{
				pti += skip;
				srclen -= skip;
			}
			else
			{
				memcpy(d->mth->telbuf, pti, srclen);
				d->mth->teltop = srclen;

				*pto = 0;
				return strlen((char*)out);
			}
			break;

		case '\r':
			if (srclen > 1 && pti[1] == '\0')
			{
				*pto++ = '\n';
			}
			pti++;
			srclen--;
			break;

		case '\0':
			pti++;
			srclen--;
			break;

		default:
			*pto++ = *pti++;
			srclen--;
			break;
		}
	}
	*pto = 0;

	// handle remote echo for windows telnet

	if (HAS_BIT(d->mth->comm_flags, COMM_FLAG_REMOTEECHO))
	{
		skip = strlen((char*)out);

		for (cnt = 0; cnt < skip; cnt++)
		{
			switch (out[cnt])
			{
			case   8:
			case 127:
				out[cnt] = '\b';
				write_to_descriptor(d, "\b \b", 3);
				break;

			case '\n':
				write_to_descriptor(d, "\r\n", 2);
				break;

			default:
				if (HAS_BIT(d->mth->comm_flags, COMM_FLAG_PASSWORD))
				{
					write_to_descriptor(d, "*", 1);
				}
				else
				{
					write_to_descriptor(d, (char*)out + cnt, 1);
				}
				break;
			}
		}
	}
	return strlen((char*)out);
}

void debug_telopts(Descriptor* d, unsigned char* src, size_t srclen)
{
	if (!cfg_get_debug_telopt())
		return;

	if (srclen > 1)
	{
		switch (src[1])
		{
		case IAC:
			log_descriptor_printf(d, "RCVD IAC IAC");
			break;

		case DO:
		case DONT:
		case WILL:
		case WONT:
		case SB:
			if (srclen > 2)
			{
				if (src[1] == SB)
				{
					if (skip_sb(d, src, srclen) == srclen + 1)
					{
						log_descriptor_printf(d, "RCVD IAC SB %s ?", TELOPT(src[2]));
					}
					else
					{
						log_descriptor_printf(d, "RCVD IAC SB %s IAC SE", TELOPT(src[2]));
					}
				}
				else
				{
					log_descriptor_printf(d, "RCVD IAC %s %s", TELCMD(src[1]), TELOPT(src[2]));
				}
			}
			else
			{
				log_descriptor_printf(d, "RCVD IAC %s ?", TELCMD(src[1]));
			}
			break;

		default:
			if (TELCMD_OK(src[1]))
			{
				log_descriptor_printf(d, "RCVD IAC %s", TELCMD(src[1]));
			}
			else
			{
				log_descriptor_printf(d, "RCVD IAC %d", src[1]);
			}
			break;
		}
	}
	else
	{
		log_descriptor_printf(d, "RCVD IAC ?");
	}
}

/*
	Send to client to have it disable local echo
*/

void send_echo_off(Descriptor* d)
{
	SET_BIT(d->mth->comm_flags, COMM_FLAG_PASSWORD);

	descriptor_printf(d, "%c%c%c", IAC, WILL, TELOPT_ECHO);
}

/*
	Send to client to have it enable local echo
*/

void send_echo_on(Descriptor* d)
{
	DEL_BIT(d->mth->comm_flags, COMM_FLAG_PASSWORD);

	descriptor_printf(d, "%c%c%c", IAC, WONT, TELOPT_ECHO);
}

/*
	Send right after the prompt to mark it as such.
*/

void send_eor(Descriptor* d)
{
	if (HAS_BIT(d->mth->comm_flags, COMM_FLAG_EOR))
	{
		descriptor_printf(d, "%c%c", IAC, EOR);
	}
}

/*
	End Of Record negotiation - not enabled by default in tables.c
*/

size_t process_do_eor(Descriptor* d, unsigned char* src, size_t srclen)
{
	SET_BIT(d->mth->comm_flags, COMM_FLAG_EOR);

	return 3;
}

/*
	Terminal Type negotiation - make sure d->mth->terminal_type is initialized.
*/

size_t process_will_ttype(Descriptor* d, unsigned char* src, size_t srclen)
{
	if (*d->mth->terminal_type == 0)
	{
		// Request the first three terminal types to see if MTTS is supported, next reset to default.

		descriptor_printf(d, "%c%c%c%c%c%c", IAC, SB, TELOPT_TTYPE, ENV_SEND, IAC, SE);
		descriptor_printf(d, "%c%c%c%c%c%c", IAC, SB, TELOPT_TTYPE, ENV_SEND, IAC, SE);
		descriptor_printf(d, "%c%c%c%c%c%c", IAC, SB, TELOPT_TTYPE, ENV_SEND, IAC, SE);
		descriptor_printf(d, "%c%c%c", IAC, DONT, TELOPT_TTYPE);
	}
	return 3;
}

size_t process_sb_ttype_is(Descriptor* d, unsigned char* src, size_t srclen)
{
	char val[MAX_INPUT_LENGTH] = { 0 };
	char* pto;
	size_t i;

	if (skip_sb(d, src, srclen) > srclen)
	{
		return srclen + 1;
	}

	pto = val;

	for (i = 4; i < srclen && src[i] != SE; i++)
	{
		switch (src[i])
		{
		default:
			*pto++ = src[i];
			break;

		case IAC:
			*pto = 0;

			if (cfg_get_debug_telopt())
			{
				log_descriptor_printf(d, "INFO IAC SB TTYPE RCVD VAL %s.", val);
			}

			if (*d->mth->terminal_type == 0)
			{
				RESTRING(d->mth->terminal_type, val);
			}
			else
			{
#ifdef _MSC_VER
				if (sscanf(val, "MTTS %lld", &d->mth->mtts) == 1)
#else
				if (sscanf(val, "MTTS %ld", &d->mth->mtts) == 1)
#endif
				{
					if (HAS_BIT(d->mth->mtts, MTTS_FLAG_256COLORS))
					{
						SET_BIT(d->mth->comm_flags, COMM_FLAG_256COLORS);
					}

					if (HAS_BIT(d->mth->mtts, MTTS_FLAG_TRUECOLOR)) {
						SET_BIT(d->mth->comm_flags, COMM_FLAG_TRUECOLOR);
					}

					if (HAS_BIT(d->mth->mtts, MTTS_FLAG_UTF8))
					{
						SET_BIT(d->mth->comm_flags, COMM_FLAG_UTF8);
					}
				}

				if (strstr(val, "-256color") || strstr(val, "-256COLOR") || strcasecmp(val, "xterm"))
				{
					SET_BIT(d->mth->comm_flags, COMM_FLAG_256COLORS);
				}
			}
			break;
		}
	}
	return i + 1;
}

/*
	NAWS: Negotiate About Window Size
*/

size_t process_sb_naws(Descriptor* d, unsigned char* src, size_t srclen)
{
	size_t i, j;

	d->mth->cols = d->mth->rows = 0;

	if (skip_sb(d, src, srclen) > srclen)
	{
		return srclen + 1;
	}

	for (i = 3, j = 0; i < srclen && j < 4; i++, j++)
	{
		switch (j)
		{
		case 0:
			d->mth->cols += (src[i] == IAC) ? src[i++] * 256 : src[i] * 256;
			break;
		case 1:
			d->mth->cols += (src[i] == IAC) ? src[i++] : src[i];
			break;
		case 2:
			d->mth->rows += (src[i] == IAC) ? src[i++] * 256 : src[i] * 256;
			break;
		case 3:
			d->mth->rows += (src[i] == IAC) ? src[i++] : src[i];
			break;
		}
	}

	if (cfg_get_debug_telopt())
	{
		log_descriptor_printf(d, "INFO IAC SB NAWS RCVD ROWS %d COLS %d", d->mth->rows, d->mth->cols);
	}

	return skip_sb(d, src, srclen);
}

/*
	NEW ENVIRON, used here to discover Windows telnet.
*/

size_t process_will_new_environ(Descriptor* d, unsigned char* src, size_t srclen)
{
	descriptor_printf(d, "%c%c%c%c%c%s%c%c", IAC, SB, TELOPT_NEW_ENVIRON, ENV_SEND, ENV_VAR, "SYSTEMTYPE", IAC, SE);

	return 3;
}

size_t process_sb_new_environ(Descriptor* d, unsigned char* src, size_t srclen)
{
	char var[MAX_INPUT_LENGTH] = { 0 };
	char val[MAX_INPUT_LENGTH] = { 0 };
	char* pto;
	size_t i;

	if (skip_sb(d, src, srclen) > srclen)
	{
		return srclen + 1;
	}

	i = 4;

	while (i < srclen && src[i] != SE)
	{
		switch (src[i])
		{
		case ENV_VAR:
		case ENV_USR:
			i++;
			pto = var;

			while (i < srclen && src[i] >= 32 && src[i] != IAC)
			{
				*pto++ = src[i++];
			}
			*pto = 0;

			if (src[i] != ENV_VAL)
			{
				log_descriptor_printf(d, "INFO IAC SB NEW-ENVIRON RCVD %d VAR %s", src[3], var);
			}
			break;

		case ENV_VAL:
			i++;
			pto = val;

			while (i < srclen && src[i] >= 32 && src[i] != IAC)
			{
				*pto++ = src[i++];
			}
			*pto = 0;

			if (cfg_get_debug_telopt())
			{
				log_descriptor_printf(d, "INFO IAC SB NEW-ENVIRON RCVD %d VAR %s VAL %s", src[3], var, val);
			}

			if (src[3] == ENV_IS)
			{
				// Detect Windows telnet and enable remote echo.

				if (!strcasecmp(var, "SYSTEMTYPE") && !strcasecmp(val, "WIN32"))
				{
					if (!strcasecmp(d->mth->terminal_type, "ANSI"))
					{
						SET_BIT(d->mth->comm_flags, COMM_FLAG_REMOTEECHO);

						RESTRING(d->mth->terminal_type, "WINDOWS TELNET");
					}
				}

				// Get the real IP address when connecting to mudportal and other MTTS compliant proxies.

				if (!strcasecmp(var, "IPADDRESS"))
				{
					RESTRING(d->mth->proxy, val);
				}
			}
			break;

		default:
			i++;
			break;
		}
	}
	return i + 1;
}

/*
	CHARSET, used to detect UTF-8 support
*/

size_t process_do_charset(Descriptor* d, unsigned char* src, size_t srclen)
{
	descriptor_printf(d, "%c%c%c%c%c%s%c%c", IAC, SB, TELOPT_CHARSET, CHARSET_REQUEST, ' ', "UTF-8", IAC, SE);

	return 3;
}

size_t process_sb_charset(Descriptor* d, unsigned char* src, size_t srclen)
{
	char val[MAX_INPUT_LENGTH] = { 0 };
	char* pto;
	size_t i;

	if (skip_sb(d, src, srclen) > srclen)
	{
		return srclen + 1;
	}

	i = 5;

	while (i < srclen && src[i] != SE && src[i] != src[4])
	{
		pto = val;

		while (i < srclen && src[i] != src[4] && src[i] != IAC)
		{
			*pto++ = src[i++];
		}
		*pto = 0;

		if (cfg_get_debug_telopt())
		{
			log_descriptor_printf(d, "INFO IAC SB CHARSET RCVD %d VAL %s", src[3], val);
		}

		if (src[3] == CHARSET_ACCEPTED)
		{
			if (!strcasecmp(val, "UTF-8"))
			{
				SET_BIT(d->mth->comm_flags, COMM_FLAG_UTF8);
			}
		}
		else if (src[3] == CHARSET_REJECTED)
		{
			if (!strcasecmp(val, "UTF-8"))
			{
				DEL_BIT(d->mth->comm_flags, COMM_FLAG_UTF8);
			}
		}
		i++;
	}
	return i + 1;
}

/*
	MSDP: Mud Server Status Protocol

	http://tintin.sourceforge.net/msdp
*/

size_t process_do_msdp(Descriptor* d, unsigned char* src, size_t srclen)
{
	int index;

	if (d->mth->msdp_data || !cfg_get_msdp_enabled())
	{
		return 3;
	}

	d->mth->msdp_data = (struct msdp_data**)alloc_mem((size_t)mud->msdp_table_size * sizeof(struct msdp_data*));

	for (index = 0; index < mud->msdp_table_size; index++)
	{
		d->mth->msdp_data[index] = (struct msdp_data*)alloc_mem(sizeof(struct msdp_data));

		d->mth->msdp_data[index]->flags = msdp_table[index].flags;
		d->mth->msdp_data[index]->value = str_dup("");
	}

	log_descriptor_printf(d, "INFO MSDP INITIALIZED");

	// Easiest to handle variable initialization here.

	msdp_update_var(d, "SPECIFICATION", "%s", "http://tintin.sourceforge.net/msdp");

	return 3;
}

size_t process_sb_msdp(Descriptor* d, unsigned char* src, size_t srclen)
{
	char var[MAX_INPUT_LENGTH] = { 0 };
	char val[MAX_INPUT_LENGTH] = { 0 };
	char* pto;
	size_t i;
	int nest;

	if (skip_sb(d, src, srclen) > srclen)
	{
		return srclen + 1;
	}

	i = 3;
	nest = 0;

	while (i < srclen && src[i] != SE)
	{
		switch (src[i])
		{
		case MSDP_VAR:
			i++;
			pto = var;

			while (i < srclen && src[i] != MSDP_VAL && src[i] != IAC)
			{
				*pto++ = src[i++];
			}
			*pto = 0;

			break;

		case MSDP_VAL:
			i++;
			pto = val;

			while (i < srclen && src[i] != IAC)
			{
				if (src[i] == MSDP_TABLE_OPEN || src[i] == MSDP_ARRAY_OPEN)
				{
					nest++;
				}
				else if (src[i] == MSDP_TABLE_CLOSE || src[i] == MSDP_ARRAY_CLOSE)
				{
					nest--;
				}
				else if (nest == 0 && (src[i] == MSDP_VAR || src[i] == MSDP_VAL))
				{
					break;
				}
				*pto++ = src[i++];
			}
			*pto = 0;

			if (nest == 0)
			{
				process_msdp_varval(d, var, val);
			}
			break;

		default:
			i++;
			break;
		}
	}
	return i + 1;
}

// MSDP over GMCP

size_t process_do_gmcp(Descriptor* d, unsigned char* src, size_t srclen)
{
	if (!cfg_get_gmcp_enabled() || d->mth->msdp_data)
	{
		return 3;
	}
	log_descriptor_printf(d, "INFO MSDP OVER GMCP INITIALIZED");

	SET_BIT(d->mth->comm_flags, COMM_FLAG_GMCP);

	return process_do_msdp(d, src, srclen);
}

size_t process_sb_gmcp(Descriptor* d, unsigned char* src, size_t srclen)
{
	char out[MAX_INPUT_LENGTH];
	size_t outlen, skiplen;

	skiplen = skip_sb(d, src, srclen);

	if (skiplen > srclen)
	{
		return srclen + 1;
	}

	outlen = json2msdp(src, srclen, out);

	process_sb_msdp(d, (unsigned char*)out, outlen);

	return skiplen;
}

/*
	MSSP: Mud Server Status Protocol

	http://tintin.sourceforge.net/mssp

	Uncomment and update as needed
*/

size_t process_do_mssp(Descriptor* d, unsigned char* src, size_t srclen)
{
	char buffer[MAX_STRING_LENGTH] = { 0 };

	if (!cfg_get_mssp_enabled())
		return 3;

	int player_count = 0;

	PlayerData* player_next = NULL;
	for (PlayerData* player = player_data_list; player != NULL; player = player_next) {
		player_next = player->next;
		if (player->ch->invis_level > 0)
			continue;
		++player_count;
	}

	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "NAME",				MSSP_VAL,	cfg_get_mud_name());
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "PLAYERS",			MSSP_VAL,	player_count);

	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "UPTIME",				MSSP_VAL,	get_uptime());

	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "HOSTNAME",			MSSP_VAL,	cfg_get_hostname());

	if (cfg_get_telnet_enabled())
		cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "PORT",			MSSP_VAL,	cfg_get_telnet_port());

	if (cfg_get_tls_enabled())
		cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "TLS",			MSSP_VAL,	cfg_get_tls_port());

	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "CODEBASE",			MSSP_VAL,	cfg_get_codebase());
	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "CONTACT",			MSSP_VAL,	cfg_get_contact());
	if (cfg_get_discord()[0])
		cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "DISCORD",		MSSP_VAL,	cfg_get_discord());
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "CRAWL DELAY",		MSSP_VAL,	"-1");
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "CREATED",			MSSP_VAL,	cfg_get_created());
	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "ICON",				MSSP_VAL,	cfg_get_icon());
	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "LANGUAGE",			MSSP_VAL,	cfg_get_language());
	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "LOCATION",			MSSP_VAL,	cfg_get_location());
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MINIMUM AGE",		MSSP_VAL,	cfg_get_min_age());
	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "WEBSITE",			MSSP_VAL,	cfg_get_website());

	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "FAMILY",				MSSP_VAL,	cfg_get_family());
	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "GENRE",				MSSP_VAL,	cfg_get_genre());
	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "GAMEPLAY",			MSSP_VAL,	cfg_get_gameplay());
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "GAMESYSTEM",			MSSP_VAL,	"Custom");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "INTERMUD",			MSSP_VAL,	"");
	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "STATUS",				MSSP_VAL,	cfg_get_status());
	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "SUBGENRE",			MSSP_VAL,	cfg_get_subgenre());

	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "AREAS",				MSSP_VAL,	global_areas.count);
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "HELPFILES",			MSSP_VAL,	help_count);
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MOBILES",			MSSP_VAL,	mob_proto_count);
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "OBJECTS",			MSSP_VAL,	obj_proto_count);
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "ROOMS",				MSSP_VAL,	room_data_count);

	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "CLASSES",			MSSP_VAL,	class_count);
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "LEVELS",				MSSP_VAL,	MAX_LEVEL);
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "RACES",				MSSP_VAL,	race_count);
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "SKILLS",				MSSP_VAL,	skill_count);

	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "CHARSET",			MSSP_VAL,	"ASCII");
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "ANSI",				MSSP_VAL,	1);
#ifndef NO_ZLIB
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MCCP",				MSSP_VAL,	1);
#else
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MCCP",				MSSP_VAL,	0);
#endif
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MCP",				MSSP_VAL,	0);
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MSDP",				MSSP_VAL,	1);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MSP",				MSSP_VAL,	0);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MXP",				MSSP_VAL,	0);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "PUEBLO",				MSSP_VAL,	0);
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "UTF-8",				MSSP_VAL,	0);
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "VT100",				MSSP_VAL,	1);
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "XTERM 256 COLORS",	MSSP_VAL,	1);
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "XTERM TRUE COLORS",	MSSP_VAL,	1);

//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "PAY TO PLAY",		MSSP_VAL,	0);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "PAY FOR PERKS",		MSSP_VAL,	0);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "HIRING BUILDERS",	MSSP_VAL,	0);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "HIRING CODERS",		MSSP_VAL,	0);

	descriptor_printf(d, "%c%c%c%s%c%c", IAC, SB, TELOPT_MSSP, buffer, IAC, SE);

	return 3;
}

#ifndef NO_ZLIB

/*
	MCCP: Mud Client Compression Protocol
*/

void* zlib_alloc(void* opaque, unsigned int items, unsigned int size)
{
	int* magic = 0;
	uintptr_t ptr = (uintptr_t)alloc_mem((size_t)items * (size_t)size);
	int* prefix = (int*)(ptr - sizeof(*magic));
	*prefix += size;
	return (void*)ptr;
}

void zlib_free(void* opaque, void* address)
{
	int* magic = 0;
	uintptr_t ptr = (uintptr_t)address;
	int* size_ptr = (int*)(ptr - sizeof(*magic));
	int i_size = (*size_ptr - MAGIC_NUM);
	*size_ptr = MAGIC_NUM;	// Or free_mem() will squawk.
	size_t s_size = (size_t)i_size;
	free_mem(address, s_size);
}

#endif // !NO_ZLIB

bool start_mccp2(Descriptor* d)
{
#ifndef NO_ZLIB

	z_stream* stream;

	if (d->mth->mccp2)
	{
		return true;
	}

	stream = (z_stream*)alloc_mem(sizeof(z_stream));

	stream->next_in = NULL;
	stream->avail_in = 0;

	stream->next_out = mud->mccp_buf;
	stream->avail_out = (uInt)mud->mccp_len;

	stream->data_type = Z_ASCII;
	stream->zalloc = zlib_alloc;
	stream->zfree = zlib_free;
	stream->opaque = Z_NULL;

	/*
		12, 5 = 32K of memory, more than enough
	*/

	if (deflateInit2(stream, Z_BEST_COMPRESSION, Z_DEFLATED, 12, 5, Z_DEFAULT_STRATEGY) != Z_OK)
	{
		log_descriptor_printf(d, "start_mccp2: failed deflateInit2");
		free_mem(stream, sizeof(z_stream));

		return false;
	}

	descriptor_printf(d, "%c%c%c%c%c", IAC, SB, TELOPT_MCCP2, IAC, SE);

	/*
		The above call must send all pending output to the descriptor, since from now on we'll be compressing.
	*/

	d->mth->mccp2 = stream;

	return true;
#else
	return false;
#endif // NO_ZLIB
}


void end_mccp2(Descriptor* d)
{
#ifndef NO_ZLIB

	if (!d || !d->mth || !d->mth->mccp2)
		return;

	d->mth->mccp2->next_in = NULL;
	d->mth->mccp2->avail_in = 0;

	d->mth->mccp2->next_out = mud->mccp_buf;
	d->mth->mccp2->avail_out = (uInt)mud->mccp_len;

	if (deflate(d->mth->mccp2, Z_FINISH) != Z_STREAM_END)
	{
		log_descriptor_printf(d, "end_mccp2: failed to deflate");
	}

	if (!HAS_BIT(d->mth->comm_flags, COMM_FLAG_DISCONNECT))
	{
		process_mccp2(d);
	}

	if (deflateEnd(d->mth->mccp2) != Z_OK)
	{
		log_descriptor_printf(d, "end_mccp2: failed to deflateEnd");
	}

	free_mem(d->mth->mccp2, sizeof(z_stream));

	d->mth->mccp2 = NULL;

	log_descriptor_printf(d, "MCCP2: COMPRESSION END");

#endif // !NO_ZLIB

	return;
}

#ifndef NO_ZLIB

void write_mccp2(Descriptor* d, char* txt, size_t length)
{
	d->mth->mccp2->next_in = (unsigned char*)txt;
	d->mth->mccp2->avail_in = (uInt)length;

	d->mth->mccp2->next_out = (unsigned char*)mud->mccp_buf;
	d->mth->mccp2->avail_out = (uInt)mud->mccp_len;

	if (deflate(d->mth->mccp2, Z_SYNC_FLUSH) != Z_OK) {
		return;
	}

	process_mccp2(d);

	return;
}

#endif

static int write_sock(Descriptor* d, char* buf, size_t len)
{
#ifndef NO_OPENSSL
	size_t s_bytes = 0;
	if (d->client->type == SOCK_TLS) {
		SSL_write_ex(((TlsClient*)d->client)->ssl, buf, len, &s_bytes);
		return (int)s_bytes;
	}
	else {
#endif
#ifdef _MSC_VER
		return send(d->client->fd, buf, (int)len, 0);
#else
		return write(d->client->fd, buf, len);
#endif
#ifndef NO_OPENSSL
	}
#endif
}

#ifndef NO_ZLIB

void process_mccp2(Descriptor* d)
{
	if (write_sock(d, (char*)mud->mccp_buf, mud->mccp_len - d->mth->mccp2->avail_out) < 1)
	{
		perror("write in process_mccp2");
		SET_BIT(d->mth->comm_flags, COMM_FLAG_DISCONNECT);
	}
}

#endif // !NO_ZLIB

size_t process_do_mccp2(Descriptor* d, unsigned char* src, size_t srclen)
{
#ifndef NO_ZLIB

	if (!cfg_get_mccp2_enabled())
		return 3;

	start_mccp2(d);

#endif

	return 3;
}

size_t process_dont_mccp2(Descriptor* d, unsigned char* src, size_t srclen)
{
	end_mccp2(d);

	return 3;
}

// MCCP3

size_t process_do_mccp3(Descriptor* d, unsigned char* src, size_t srclen)
{
	if (!cfg_get_mccp3_enabled())
		return 3;

	return 3;
}

size_t process_sb_mccp3(Descriptor* d, unsigned char* src, size_t srclen)
{
#ifndef NO_ZLIB

	if (d->mth->mccp3)
	{
		end_mccp3(d);
	}

	d->mth->mccp3 = (z_stream*)alloc_mem(sizeof(z_stream));

	d->mth->mccp3->data_type = Z_ASCII;
	d->mth->mccp3->zalloc = zlib_alloc;
	d->mth->mccp3->zfree = zlib_free;
	d->mth->mccp3->opaque = NULL;

	if (inflateInit(d->mth->mccp3) != Z_OK)
	{
		log_descriptor_printf(d, "INFO IAC SB MCCP3 FAILED TO INITIALIZE");

		descriptor_printf(d, "%c%c%c", IAC, WONT, TELOPT_MCCP3);

		free_mem(d->mth->mccp3, sizeof(z_stream));
		d->mth->mccp3 = NULL;
	}
	else
	{
		log_descriptor_printf(d, "INFO IAC SB MCCP3 INITIALIZED");
	}

#endif

	return 5;
}

void end_mccp3(Descriptor* d)
{
#ifndef NO_ZLIB
	if (!d || !d->mth || !d->mth->mccp3)
		return;

	log_descriptor_printf(d, "MCCP3: COMPRESSION END");
	inflateEnd(d->mth->mccp3);
	free_mem(d->mth->mccp3, sizeof(z_stream));
	d->mth->mccp3 = NULL;

#endif
}


/*
	Returns the length of a telnet subnegotiation, return srclen + 1 for incomplete state.
*/

size_t skip_sb(Descriptor* d, unsigned char* src, size_t srclen)
{
	size_t i;

	for (i = 1; i < srclen; i++)
	{
		if (src[i] == SE && src[i - 1] == IAC)
		{
			return i + 1;
		}
	}

	return srclen + 1;
}



/*
	Utility function
*/

void descriptor_printf(Descriptor* d, char* fmt, ...)
{
	char buf[MAX_STRING_LENGTH];
	int size;
	va_list args;

	va_start(args, fmt);

	size = vsprintf(buf, fmt, args);

	va_end(args);

	write_to_descriptor(d, buf, (size_t)size);
}
