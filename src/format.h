////////////////////////////////////////////////////////////////////////////////
// format.h
// Screen and character formatting utilities
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__FORMAT_H
#define MUD98__FORMAT_H

char* format_string(const char* text);

//#define BENCHMARK_FMT_PROCS

#ifdef BENCHMARK_FMT_PROCS
extern long norm_elapsed;
extern long wrap_elapsed;
#endif

#endif // !MUD98__FORMAT_H
