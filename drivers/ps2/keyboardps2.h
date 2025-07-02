// keyboardps2.h - PS/2 keyboard implementation
// Created 2023/7/11 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "../../utils/ports.h"
#include "../../utils/cstd/str.h"

typedef struct {
  char *m_key;
  uint16_t i_key;
  uint16_t last_key;
  bool key_pressed;
  bool released;
} key_t;

// Last recorded scancode
extern uint16_t last_key;

key_t get_key();
key_t key_from_uint16_t(uint16_t k, bool update_last_key);

#ifdef __cplusplus
}
#endif