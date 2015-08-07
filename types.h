#ifndef TYPES_H
#define TYPES_H

#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
#error "No Cross Compiler"
#endif

#if !defined(__i386__)
#error "Not an ix86-elf compiler"
#endif

#define UNUSED(var) ((void)var)
#endif