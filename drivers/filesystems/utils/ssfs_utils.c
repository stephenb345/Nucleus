// ssfs_utils.c - Steve Studios Filesystem Utilities
// Created 2024/03/21 by Stephen Byrne

#include "ssfs_utils.h"

extern const char *device_path;
extern int device_handle;

extern uint8_t init_os;
extern bool fs_damaged;

bool ssfs_write_dev(__int128_t start, __int128_t len, void *buf) {
  bool ret = lseek(device_handle, start * 512, 0);
  if (!ret)
    write(device_handle, buf, len * 512);
  ret = lseek(device_handle, 0L, 0);
  return !ret;
}

bool ssfs_read_dev(__int128_t start, __int128_t len, void *buf) {
  bool ret = lseek(device_handle, start * 512, 0);
  if (!ret)
    read(device_handle, buf, len * 512);
  ret = lseek(device_handle, 0L, 0);
  return !ret;
}

long get_part_start() { return 0; }

long get_part_end() {
  lseek(device_handle, 0L, 2);
  long end = get_descriptor(device_handle)->offset;
  lseek(device_handle, 0L, 0);

  return end;
}

bool reload_header(uint64_t journal_size, uint64_t total_dirents,
                   uint64_t data_size, uint64_t free_block_table_size,
                   uint64_t free_dirent_table_size,
                   uint64_t free_inode_table_size,
                   uint64_t dirent_inode_table_byte_size,
                   uint64_t inode_shared_data_table_byte_size,
                   bool write_backwards) {
  bool ret;

  ssfs_header *header = malloc(sizeof(ssfs_header));
  memset(header, 0, sizeof(ssfs_header));

  header->ssfs_magic_number = 0x11462952;
  header->journal_size = journal_size;
  header->data_used = data_size;
  header->total_sectors = get_part_end() - get_part_start();
  header->total_dirents = total_dirents;
  header->free_block_table_size = free_block_table_size;
  header->free_inode_table_size = free_inode_table_size;
  header->free_dirent_table_size = free_dirent_table_size;
  header->dirent_inode_table_byte_size = dirent_inode_table_byte_size;
  header->inode_shared_data_table_byte_size = inode_shared_data_table_byte_size;
  header->write_backwards = write_backwards;
  header->damaged = fs_damaged;
  header->posix_last_mount = 0;
  header->posix_last_write = 0xFF;
  header->origin_os = 0x0;

  ret = !ssfs_write_dev(get_part_start(), 1, header);
  ret = !ssfs_write_dev(get_part_end(), 1, header);

  free(header);
  return ret;
}

bool reload_journal(ssfs_journal_broken_data *broken_data,
                    ssfs_journal_orphaned_dirents *orphaned_dirents,
                    ssfs_journal_cached_data *cached_data,
                    uint32_t broken_data_size, uint32_t orphaned_dirents_size,
                    uint32_t cached_data_size) {
  bool ret;

  char sh_buf[SECTOR_SIZE];
  ret = !ssfs_read_dev(get_part_start(), 1, sh_buf);

  ssfs_header *sh = malloc(sizeof(ssfs_header));
  memcpy(sh, sh_buf, sizeof(ssfs_header));

  ssfs_journal *journal =
      malloc(sizeof(ssfs_journal) +
             (broken_data->broken_dirents * sizeof(ssfs_dirent)) +
             (orphaned_dirents->orphaned_dirents * sizeof(ssfs_inode)) +
             (cached_data->cached_data_size * sizeof(ssfs_data_loc)));
  journal->broken_data = *broken_data;
  journal->orphaned_dirents = *orphaned_dirents;
  journal->cached_data = *cached_data;

  uint32_t j_size = sizeof(ssfs_journal) +
                    (broken_data->broken_dirents * sizeof(ssfs_dirent)) +
                    (orphaned_dirents->orphaned_dirents * sizeof(ssfs_inode)) +
                    (cached_data->cached_data_size * sizeof(ssfs_data_loc));
  ret = reload_header(
      j_size, sh->total_dirents, sh->data_used, sh->free_block_table_size,
      sh->free_inode_table_size, sh->free_dirent_table_size,
      sh->dirent_inode_table_byte_size, sh->inode_shared_data_table_byte_size,
      sh->write_backwards);

  ret = !ssfs_read_dev(get_part_start(), 1, sh_buf);
  memcpy(sh, sh_buf, sizeof(ssfs_header));

  journal->broken_data_size = broken_data_size;
  journal->orphaned_dirents_size = orphaned_dirents_size;
  journal->cached_data_size = cached_data_size;

  ret = !ssfs_write_dev(get_part_end() -
                            ((j_size + SECTOR_SIZE) / SECTOR_SIZE) - 1,
                        (j_size + SECTOR_SIZE) / SECTOR_SIZE, journal);

  free(sh);
  free(journal);

  return ret;
}

#define JOURNAL_STATIC_FIELDS_U32 3
#define JOURNAL_TABLES_START (sizeof(uint32_t) * JOURNAL_STATIC_FIELDS_U32)

ssfs_journal *read_journal() {
  char sh_buf[SECTOR_SIZE];
  ssfs_read_dev(get_part_start(), 1, sh_buf);

  ssfs_header *sh = malloc(sizeof(ssfs_header));
  memcpy(sh, sh_buf, sizeof(ssfs_header));

  char j_buf[((sh->journal_size + SECTOR_SIZE) / SECTOR_SIZE) * SECTOR_SIZE];
  ssfs_read_dev(get_part_end() -
                    ((sh->journal_size + SECTOR_SIZE) / SECTOR_SIZE) - 1,
                (sh->journal_size + SECTOR_SIZE) / SECTOR_SIZE, j_buf);

  ssfs_journal *j = malloc(sh->journal_size);
  memcpy(j, j_buf, sh->journal_size);

  free(sh);

  char broken_data_buf[j->broken_data_size + sizeof(uint32_t)];
  char orphaned_dirent_buf[j->orphaned_dirents_size + sizeof(uint32_t)];
  char cached_data_buf[j->cached_data_size + sizeof(uint32_t)];

  int index = 0;
  for (int i = JOURNAL_TABLES_START;
       i < JOURNAL_TABLES_START + j->broken_data_size + sizeof(uint32_t); i++) {
    broken_data_buf[index] = j_buf[i];
    index++;
  }

  index = 0;
  for (int i = JOURNAL_TABLES_START + j->broken_data_size + sizeof(uint32_t);
       i < JOURNAL_TABLES_START + j->broken_data_size + sizeof(uint32_t) +
               j->orphaned_dirents_size + sizeof(uint32_t);
       i++) {
    orphaned_dirent_buf[index] = j_buf[i];
    index++;
  }

  index = 0;
  for (int i = JOURNAL_TABLES_START + j->broken_data_size + sizeof(uint32_t) +
               j->orphaned_dirents_size + sizeof(uint32_t);
       i < JOURNAL_TABLES_START + j->broken_data_size + sizeof(uint32_t) +
               j->orphaned_dirents_size + sizeof(uint32_t) +
               j->cached_data_size + sizeof(uint32_t);
       i++) {
    cached_data_buf[index] = j_buf[i];
    index++;
  }

  j->broken_data = *(ssfs_journal_broken_data *)broken_data_buf;
  j->orphaned_dirents = *(ssfs_journal_orphaned_dirents *)orphaned_dirent_buf;
  j->cached_data = *(ssfs_journal_cached_data *)cached_data_buf;

  return j;
}

ssfs_inode *to_inode(char *data, int data_size, uint32_t shared_data_size) {
  ssfs_inode *inode = malloc(data_size);

  char shared_data_buf[(shared_data_size * sizeof(ssfs_data_loc)) +
                       sizeof(ssfs_inode)];
  char data_loc_table_buf[data_size -
                          ((shared_data_size * sizeof(ssfs_data_loc)) -
                           sizeof(ssfs_inode))];

  int index = 0;
  int i;

  for (i = sizeof(ssfs_inode);
       i < sizeof(ssfs_inode) + (shared_data_size * sizeof(ssfs_data_loc));
       i++) {
    shared_data_buf[index] = data[i];
    index++;
  }

  index = 0;
  for (; i < data_size; i++) {
    data_loc_table_buf[index] = data[i];
    index++;
  }

  inode->shared_data.shared_data_size = shared_data_size;
  inode->data_loc_size =
      (data_size - (shared_data_size * sizeof(ssfs_data_loc)) -
       sizeof(ssfs_inode)) /
      sizeof(ssfs_data_loc);

  memcpy(inode->shared_data.shared_data_locations, shared_data_buf,
         shared_data_size * sizeof(ssfs_data_loc));
  memcpy(inode->data_loc_table, data_loc_table_buf,
         inode->data_loc_size * sizeof(ssfs_data_loc));

  return inode;
}

ssfs_shared_data_info ssfs_get_dirent_shared_data(char data[],
                                                  ssfs_dirent dirent_table[],
                                                  uint32_t fragment_index) {
  uint32_t data_size = strlen(data);

  bool __contains_shared_data = false;
  ssfs_shared_data_object **shared_data =
      malloc(sizeof(ssfs_shared_data_object *));

  uint32_t shared_data_byte_size = 0;
  uint32_t shared_data_size = 0;

  char *__updated_raw_data[strlen(data)];

  for (uint32_t i = 0; i < strlen(data); i++) {
    __updated_raw_data[i] = "\0";
  }

  char sh_buf[SECTOR_SIZE];
  ssfs_read_dev(get_part_start(), 1, sh_buf);

  ssfs_header *sh = (ssfs_header *)sh_buf;
  ssfs_shared_data_info ret;
  ret.shared_data_len = 0;

  uint32_t files_checked = 0;
  for (uint32_t i = 0; i < sh->total_dirents; i++) {
    if (dirent_table[i].type != 2)
      continue;

    files_checked++;
    char dirent_inode_buf[((dirent_table[i].inode_loc.size + SECTOR_SIZE) /
                           SECTOR_SIZE) *
                          SECTOR_SIZE];
    ssfs_read_dev(dirent_table[i].inode_loc.start_lba,
                  (dirent_table[i].inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE,
                  dirent_inode_buf);

    char final_buf[(((dirent_table[i].inode_loc.size + SECTOR_SIZE) /
                     SECTOR_SIZE) *
                    SECTOR_SIZE) -
                   dirent_table[i].inode_loc.start_byte];
    uint32_t i = 0;
    uint32_t index = 0;
    for (i = dirent_table[i].inode_loc.start_byte;
         i < ((dirent_table[i].inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE) *
                 SECTOR_SIZE;
         i++) {
      final_buf[index] = dirent_inode_buf[i];
      index++;
    }
    ssfs_inode *file_info = (ssfs_inode *)final_buf;

    char target_buf[(data_size / SECTOR_SIZE) * SECTOR_SIZE];
    ssfs_read_dev(file_info->data_loc_table[fragment_index].start_lba,
                  data_size / SECTOR_SIZE, target_buf);

    char *data_entries[data_size];

    uint32_t size = 0;
    uint32_t dirent_num = 0;
    uint32_t raw_data_size = 0;

    uint32_t last_match_loc = 0;

    for (uint32_t j = 0; j < data_size; j++) {
      size++;

      __updated_raw_data[raw_data_size] = &target_buf[j];
      raw_data_size++;

      if ((size % 4 == 0 && size >= 32) &&
          (j - last_match_loc) <= (0x100000 * 512) /* 512 MiB*/) {
        char buf[size];
        char buf2[size];

        for (uint32_t k = j; k < size; k++) {
          buf[k] = data[k];
          buf2[k] = target_buf[j];
        }

        if (memcmp(buf, buf2, size)) {
          bool contains_shared_data = false;
          uint32_t dirent_containing_duplicate_data = 0;
          uint32_t times_seen = 0;

          for (uint32_t k = 0; k < dirent_num; k++) {
            if (memcmp(data_entries[dirent_num], buf, size)) {
              contains_shared_data = true;
              dirent_containing_duplicate_data = k;
              times_seen++;
              break;
            }
          }

          if (contains_shared_data) {
            shared_data[dirent_num] =
                realloc(shared_data[dirent_num],
                        (sizeof(ssfs_shared_data_object) +
                         (sizeof(uint64_t) *
                          shared_data[dirent_containing_duplicate_data]
                              ->index_table_size)));

            shared_data[dirent_containing_duplicate_data]->index_table_size++;
            shared_data[dirent_containing_duplicate_data]->indexes[times_seen] =
                j;

            break;
          }

          shared_data =
              realloc(shared_data, shared_data_byte_size +
                                       sizeof(ssfs_shared_data_object *));
          shared_data_byte_size += sizeof(ssfs_shared_data_object *);

          shared_data[dirent_num] =
              malloc(sizeof(ssfs_shared_data_object) + sizeof(uint64_t));

          shared_data[dirent_num]->data_loc.start_lba =
              (data_size + j +
               file_info->data_loc_table[fragment_index].start_lba) /
              SECTOR_SIZE;
          shared_data[dirent_num]->data_loc.start_byte = j;
          shared_data[dirent_num]->data_loc.size = size;

          shared_data[dirent_num]->index_table_size++;
          shared_data[dirent_num]->indexes[0] = j;

          shared_data_size += size;

          data_entries[dirent_num] = buf;
          dirent_num++;

          size = 0;

          __contains_shared_data = true;
          last_match_loc = j;

          for (uint32_t k = raw_data_size; k > raw_data_size - size; k--) {
            __updated_raw_data[k] = "\0";
          }
        }
      }
    }
    ret.shared_data_len = dirent_num;
    if (files_checked >= 128)
      break;
  }
  uint32_t final_size = 0;
  for (uint32_t i = 0; i < strlen(data); i++) {
    if (strcmp(__updated_raw_data[i], (char *)"\0"))
      final_size++;
  }

  char updated_raw_data[final_size];

  for (uint32_t i = 0; i < strlen(data); i++) {
    if (strcmp(__updated_raw_data[i], (char *)"\0"))
      updated_raw_data[i] = *__updated_raw_data[i];
  }

  ret.contains_shared_data = __contains_shared_data;
  ret.shared_data = shared_data;
  ret.shared_data_size = shared_data_size;
  ret.updated_raw_data = updated_raw_data;
  ret.updated_raw_data_size = final_size;

  free(sh);
  return ret;
}

ssfs_filename_info ssfs_get_filename_info(char path[]) {
  uint32_t last_slash_index = 0;

  ssfs_filename_info ret;
  ret.filename_size = 0;
  ret.last_slash_index = 0;

  uint32_t parents_size = 0;
  uint32_t parents_table_size = 0;
  for (uint32_t i = 0; i < strlen(path); i++) {
    ret.filename_size++;
    if (path[i] == '/') {
      ret.filename_size = 0;
      ret.last_slash_index = i;

      parents_table_size++;

      continue;
    }
    parents_size++;
  }

  char filename[ret.filename_size];
  uint32_t i, j;
  for (i = last_slash_index, j = 0;
       i < ret.filename_size && j < ret.filename_size; i++, j++) {
    filename[j] = path[i];
  }

  memcpy(ret.filename, filename, ret.filename_size);
  ret.parents_size = parents_size > 0 ? parents_size - ret.filename_size : 0;

  uint32_t sz = 0;
  uint32_t index = 0;
  char parents[parents_size][parents_table_size];

  for (uint32_t i = 0; i < parents_table_size; i++) {
    for (uint32_t j = sz; j < strlen(path); j++) {
      if (j >= ret.last_slash_index || path[j] == '/') {
        sz++;
        index = 0;
        break;
      }

      sz++;
      parents[i][index] = path[j];

      index++;
    }
  }
  ret.parents = (char **)parents;
  return ret;
}

ssfs_path_info ssfs_setup_dirent_parents(ssfs_filename_info info,
                                         ssfs_dirent dirent,
                                         ssfs_dirent dirent_table[]) {
  char sh_buf[SECTOR_SIZE];
  ssfs_read_dev(get_part_start(), 1, sh_buf);

  ssfs_header *sh = malloc(sizeof(ssfs_header));
  memcpy(sh, sh_buf, sizeof(ssfs_header));

  uint32_t dirent_table_size = sh->total_dirents;

  uint32_t dirent_index_table[info.parents_size];
  ssfs_dirent dirent_parents_table[info.parents_size];

  for (uint32_t i = 0; i < info.parents_size; i++) {
    for (uint32_t j = 0; j < dirent_table_size; j++) {
      if (dirent_table[j].parent_dirent_table_index == 0)
        continue;
      if (memcmp(dirent_table[j].name, info.parents[i], 256)) {
        for (uint32_t k = i; k < info.parents_size; k++) {
          for (uint32_t l = j; l < dirent_table_size; l++) {
            ssfs_dirent dirent_parent = dirent_table[j];
            if (!memcmp(
                    dirent_table[dirent_parent.parent_dirent_table_index].name,
                    info.parents[j], 256))
              continue;

            dirent_index_table[k] = l;
            dirent_parents_table[k] = dirent_parent;
            break;
          }
        }
      }
      break;
    }
  }

  ssfs_dirent ret[dirent_table_size];
  memcpy(ret, dirent_table, dirent_table_size * sizeof(ssfs_dirent));

  for (uint32_t i = 0; i < info.parents_size; i++) {
    dirent_parents_table[i].parent_dirent_table_index = dirent_index_table[i];
    char m_dirent_inode_buf[((dirent_parents_table[i].inode_loc.size +
                              SECTOR_SIZE) /
                             SECTOR_SIZE) *
                            SECTOR_SIZE];
    ssfs_read_dev(dirent_parents_table[i].inode_loc.start_lba,
                  (dirent_parents_table[i].inode_loc.size + SECTOR_SIZE) /
                      SECTOR_SIZE,
                  m_dirent_inode_buf);

    char m_final_buf[(((dirent_parents_table[i].inode_loc.size + SECTOR_SIZE) /
                       SECTOR_SIZE) *
                      SECTOR_SIZE) -
                     dirent_parents_table[i].inode_loc.start_byte];
    uint32_t i = 0;
    uint32_t index = 0;
    for (i = dirent_parents_table[i].inode_loc.start_byte;
         i < ((dirent_parents_table[i].inode_loc.size + SECTOR_SIZE) /
              SECTOR_SIZE) *
                 SECTOR_SIZE;
         i++) {
      m_final_buf[index] = m_dirent_inode_buf[i];
      index++;
    }
    ssfs_inode *m_file_info = (ssfs_inode *)m_final_buf;

    for (uint32_t j = 0; j < dirent_table_size; j++) {
      char dirent_inode_buf[((ret[j].inode_loc.size + SECTOR_SIZE) /
                             SECTOR_SIZE) *
                            SECTOR_SIZE];
      ssfs_read_dev(ret[j].inode_loc.start_lba,
                    (ret[j].inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE,
                    dirent_inode_buf);

      char final_buf[(((ret[j].inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE) *
                      SECTOR_SIZE) -
                     ret[j].inode_loc.start_byte];
      uint32_t i;
      uint32_t index = 0;
      for (i = ret[j].inode_loc.start_byte;
           i <
           ((ret[j].inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE) * SECTOR_SIZE;
           i++) {
        final_buf[index] = dirent_inode_buf[i];
        index++;
      }

      ssfs_inode *file_info = (ssfs_inode *)final_buf;

      if (file_info->data_loc_size == m_file_info->data_loc_size &&
          file_info->data_loc_table == m_file_info->data_loc_table &&
          ret[j].encrypted == dirent_parents_table[i].encrypted &&
          memcmp(ret[j].name, dirent_parents_table[i].name, 256) &&
          ret[j].parent_dirent_table_index ==
              dirent_parents_table[i].parent_dirent_table_index &&
          ret[j].permissions == dirent_parents_table[i].permissions &&
          ret[j].link_dirent_table_index ==
              dirent_parents_table[i].link_dirent_table_index &&
          ret[j].name_size == dirent_parents_table[i].name_size) {
        ret[j] = dirent_parents_table[i];
      }
    }
  }

  ssfs_dirent ret_dirent;
  ret_dirent = dirent;

  if (info.parents_size == 0)
    ret_dirent.parent_dirent_table_index = 0;
  else
    ret_dirent.parent_dirent_table_index =
        dirent_index_table[0] == 0 ? 0 : dirent_index_table[0];

  ssfs_path_info m_ret;
  memcpy(m_ret.dirent_table, ret, dirent_table_size);

  m_ret.last_child = ret_dirent;

  free(sh);
  return m_ret;
}

ssfs_dirent ssfs_get_dirent_from_info(ssfs_filename_info info,
                                      ssfs_dirent dirent_table[]) {
  char sh_buf[SECTOR_SIZE];
  ssfs_read_dev(get_part_start(), 1, sh_buf);

  ssfs_header *sh = malloc(sizeof(ssfs_header));
  memcpy(sh, sh_buf, sizeof(ssfs_header));

  uint32_t dirent_table_size = sh->total_dirents;
  free(sh);

  ssfs_dirent dirent = {
      .encrypted = false,
      .inode_loc = {.size = 0, .start_byte = 0, .start_lba = 0},
      .is_link = false,
      .link_dirent_table_index = 0,
      .link_type = 0,
      .name = "\0",
      .name_size = 0,
      .parent_dirent_table_index = 0,
      .permissions = 0000,
      .type = 0x0,
  };

  uint32_t i;

  for (i = 0; i < info.parents_size; i++) {
    for (uint32_t j = 0; j < dirent_table_size; j++) {
      if (dirent_table[j].parent_dirent_table_index == 0)
        continue;
      if (memcmp(dirent_table[j].name, info.filename, 256) &&
          memcmp(dirent_table[dirent_table[j].parent_dirent_table_index].name,
                 info.parents[i], 256)) {
        return dirent_table[j];
      }
    }
  }

  if (info.parents_size == 0) {
    for (i = 0; i < dirent_table_size; i++) {
      if (memcmp(dirent_table[i].name, info.filename, 256) &&
          dirent_table[i].parent_dirent_table_index == 0) {
        return dirent_table[i];
      }
    }
  }

  return dirent;
}