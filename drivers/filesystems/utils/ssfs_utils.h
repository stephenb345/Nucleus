// ssfs_utils.h - Steve Studios Filesystem Utilities
// Created 2024/03/21 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../../../posix/read.h"
#include "../../../posix/write.h"
#include "../../../posix/lseek.h"
#include "../../../posix/open.h"
#include "../../../posix/close.h"

#define SECTOR_SIZE 512

typedef struct {
  uint64_t ssfs_magic_number;
  uint64_t total_sectors;
  uint64_t total_dirents;
  uint64_t data_used;
  uint64_t journal_size;
  uint64_t free_block_table_size;
  uint64_t free_dirent_table_size;
  uint64_t free_inode_table_size;
  uint64_t dirent_inode_table_byte_size;
  uint64_t inode_shared_data_table_byte_size;
  uint32_t major_version;
  uint16_t minor_version;
  uint64_t posix_last_write;
  uint64_t posix_last_mount;
  uint8_t origin_os;
  bool damaged;
  bool write_backwards;
} __attribute__((packed)) ssfs_header;

typedef struct {
  __uint128_t start_lba;
  uint64_t start_byte;
  uint32_t size;
} __attribute__((packed)) ssfs_data_loc;

typedef struct {
  ssfs_data_loc data_loc;
  uint32_t index_table_size;
  uint64_t indexes[];
} __attribute__((packed)) ssfs_shared_data_object;

typedef struct {
  uint32_t shared_data_size;
  ssfs_data_loc shared_data_locations[];
} __attribute__((packed)) ssfs_file_shared_data;

typedef struct {
  uint64_t data_loc_size;
  uint32_t user;
  uint32_t group;
  uint64_t posix_last_access_time;
  uint64_t posix_creation_time;
  uint64_t posix_last_mod_time;
  uint64_t posix_deletion_time;
  ssfs_file_shared_data shared_data;
  ssfs_data_loc data_loc_table[];
} __attribute__((packed)) ssfs_inode;

typedef struct {
  char name[256];
  uint16_t permissions;
  bool encrypted;
  bool is_link;
  char link_type;
  uint32_t name_size;
  char type;
  uint64_t parent_dirent_table_index;
  uint64_t link_dirent_table_index;
  ssfs_data_loc inode_loc;
} __attribute__((packed)) ssfs_dirent;

typedef struct {
  uint32_t broken_dirents;
  ssfs_dirent broken_dirent_table
      []; // If a crash happens while an dirent is being transferred to another
          // directory, this dirent is considered broken and is stored here
          // until the system can restore the file to the directory where it
          // would have been moved to. This table stores the full path (if the
          // dirent had been moved, which in this case it has not) of the dirent
          // as it would have been seen by the end user, and right after that,
          // it stores the data of the broken dirent.
} __attribute__((packed)) ssfs_journal_broken_data;

typedef struct {
  uint32_t orphaned_dirents;
  uint64_t orphaned_dirent_indexes[];
} __attribute__((packed)) ssfs_journal_orphaned_dirents;

typedef struct {
  uint32_t cached_data_size;
  ssfs_data_loc data_to_be_written[];
} __attribute__((packed)) ssfs_journal_cached_data;

typedef struct {
  ssfs_data_loc data_loc;
  char comp_shared_data[];
} __attribute__((packed)) ssfs_journal_shared_data_cache;

typedef struct {
  uint32_t broken_data_size;
  uint32_t orphaned_dirents_size;
  uint32_t cached_data_size;
  ssfs_journal_broken_data broken_data;
  ssfs_journal_orphaned_dirents orphaned_dirents;
  ssfs_journal_cached_data cached_data;
} __attribute__((packed)) ssfs_journal;

// Not an on-disk structure
typedef struct {
  bool contains_shared_data;
  ssfs_shared_data_object **shared_data;
  uint32_t shared_data_size;
  char *updated_raw_data;
  uint32_t updated_raw_data_size;
  uint32_t shared_data_len;
} ssfs_shared_data_info;

// Not an on-disk structure
typedef struct {
  uint32_t filename_size;
  uint32_t last_slash_index;
  char filename[256];
  uint32_t parents_size;
  char **parents;
} ssfs_filename_info;

// Not an on-disk structure
typedef struct {
  ssfs_dirent last_child;
  ssfs_dirent dirent_table[];
} ssfs_path_info;

bool reload_header(uint64_t journal_size, uint64_t total_dirents,
                   uint64_t data_size, uint64_t free_block_table_size,
                   uint64_t free_dirent_table_size,
                   uint64_t free_inode_table_size,
                   uint64_t dirent_inode_table_byte_size,
                   uint64_t inode_shared_data_table_byte_size, bool write_backwards);
bool reload_journal(ssfs_journal_broken_data *broken_data,
                    ssfs_journal_orphaned_dirents *orphaned_dirents,
                    ssfs_journal_cached_data *cached_data,
                    uint32_t broken_data_size, uint32_t orphaned_dirents_size,
                    uint32_t cached_data_size);

ssfs_journal *read_journal();
ssfs_inode *to_inode(char data[], int data_size, uint32_t shared_data_size);

ssfs_shared_data_info ssfs_get_dirent_shared_data(char data[],
                                                  ssfs_dirent dirent_table[],
                                                  uint32_t fragment_index);
ssfs_filename_info ssfs_get_filename_info(char path[]);
ssfs_path_info ssfs_setup_dirent_parents(ssfs_filename_info info,
                                         ssfs_dirent dirent,
                                         ssfs_dirent dirent_table[]);
ssfs_dirent ssfs_get_dirent_from_info(ssfs_filename_info info,
                                      ssfs_dirent dirent_table[]);

bool ssfs_read_dev(__int128_t start, __int128_t len, void* buf);
bool ssfs_write_dev(__int128_t start, __int128_t len, void* buf);

#ifdef __cplusplus
}
#endif