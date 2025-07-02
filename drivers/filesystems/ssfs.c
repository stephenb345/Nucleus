// ssfs.c - Steve Studios Filesystem Driver
// Created 2023/11/12 by Stephen Byrne

#include "ssfs.h"

int get_part_start();
int get_part_end();

bool fs_damaged = false;

file_response ssfs_read_file(char path[]) {
  bool ret;
  ssfs_filename_info info = ssfs_get_filename_info(path);

  // Read the SSFS header
  char sh_buf[SECTOR_SIZE];
  ret = !ssfs_read_dev(get_part_start(), 1, sh_buf);

  ssfs_header *sh = malloc(sizeof(ssfs_header));
  memcpy(sh, sh_buf, sizeof(ssfs_header));

  file_response tmp_resp = {.content = "\0", .size = 0};

  if (sh->total_dirents == 0)
    return tmp_resp;

  uint32_t dirent_table_size = sh->total_dirents + 1;
  uint64_t dirent_loc = get_part_start() + 1;

  // Read the SSFS dirent table
  ssfs_dirent dirent_table[dirent_table_size];
  ret = !ssfs_read_dev(
      dirent_loc,
      ((sizeof(ssfs_dirent) * sh->total_dirents) + SECTOR_SIZE) / SECTOR_SIZE,
      dirent_table);

  ssfs_dirent dirent = ssfs_get_dirent_from_info(info, dirent_table);
  uint32_t full_data_size = 0;

  // Read the inode table
  char
      m_dirent_inode_buf[((dirent.inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE) *
                         SECTOR_SIZE];
  ret = !ssfs_read_dev(dirent.inode_loc.start_lba,
                       (dirent.inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE,
                       m_dirent_inode_buf);

  // Only extract the inodes connected to this dirent
  char final_buf[dirent.inode_loc.size];

  char shared_data_size_buf[sizeof(uint32_t)];
  uint32_t shared_data_size = 0;

  uint64_t i;
  uint64_t index = 0;
  uint32_t shared_data_index = 0;

  for (i = dirent.inode_loc.start_byte;
       i < dirent.inode_loc.start_byte + dirent.inode_loc.size; i++) {
    final_buf[index] = (char)m_dirent_inode_buf[i];
    if (index >= sizeof(uint64_t) &&
        index <= sizeof(uint64_t) + sizeof(uint32_t)) {
      shared_data_size_buf[shared_data_index] = final_buf[index];
      shared_data_index++;
    }
    index++;
  }

  shared_data_size = *(uint32_t *)shared_data_size_buf;

  ssfs_inode *tmp_final_buf = malloc(index);
  memcpy(tmp_final_buf, final_buf, index);

  ssfs_inode *m_final_buf =
      to_inode(final_buf, dirent.inode_loc.size, shared_data_size);
  free(tmp_final_buf);

  for (i = 0; i < m_final_buf->data_loc_size; i++) {
    full_data_size += m_final_buf->data_loc_table[i].size;
  }
  for (i = 0; i < m_final_buf->shared_data.shared_data_size; i++) {
    char shared_data_buf[((dirent.inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE) *
                         SECTOR_SIZE];

    ret = !ssfs_read_dev(
        m_final_buf->shared_data.shared_data_locations[i].start_lba,
        (dirent.inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE, shared_data_buf);

    char final_buf[m_final_buf->shared_data.shared_data_locations[i].size];
    int index = 0;
    for (int j = m_final_buf->shared_data.shared_data_locations[i].start_byte;
         j < m_final_buf->shared_data.shared_data_locations[i].start_byte +
                 m_final_buf->shared_data.shared_data_locations[i].size;
         j++) {
      final_buf[index] = shared_data_buf[j];
      index++;
    }

    ssfs_shared_data_object *m_shared_data_object =
        (ssfs_shared_data_object *)final_buf;
    full_data_size += m_shared_data_object->data_loc.size;
  }

  // Allocate the size of the data we'll be returning
  char *all_data = malloc(full_data_size);
  memset(all_data, 0, full_data_size);

  // the fragments
  for (i = 0; i < m_final_buf->data_loc_size; i++) {
    char data_part[((m_final_buf->data_loc_table[i].size + SECTOR_SIZE) /
                    SECTOR_SIZE) *
                   SECTOR_SIZE];
    ret = !ssfs_read_dev(m_final_buf->data_loc_table[i].start_lba,
                         (m_final_buf->data_loc_table[i].size + SECTOR_SIZE) /
                             SECTOR_SIZE,
                         data_part);

    char needed_data[m_final_buf->data_loc_table[i].size];
    for (int j = 0; j < m_final_buf->data_loc_table[i].size; j++) {
      needed_data[j] = (char)data_part[j];
    }

    // Add this data to our "all_data" buffer
    strcat(all_data, needed_data);
  }

  // Add the shared data
  for (i = 0; i < m_final_buf->shared_data.shared_data_size; i++) {
    char shared_data_buf[((dirent.inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE) *
                         SECTOR_SIZE];

    ret = !ssfs_read_dev(

        m_final_buf->shared_data.shared_data_locations[i].start_lba,
        (dirent.inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE, shared_data_buf);

    char final_buf[m_final_buf->shared_data.shared_data_locations[i].size];
    int index = 0;
    for (int j = m_final_buf->shared_data.shared_data_locations[i].start_byte;
         j < m_final_buf->shared_data.shared_data_locations[i].start_byte +
                 m_final_buf->shared_data.shared_data_locations[i].size;
         j++) {
      final_buf[index] = shared_data_buf[j];
      index++;
    }

    ssfs_shared_data_object *m_shared_data_object =
        (ssfs_shared_data_object *)final_buf;
    full_data_size += m_shared_data_object->data_loc.size;
    for (int j = 0; j < m_shared_data_object->index_table_size; j++) {
      uint32_t k;
      for (k = m_shared_data_object->indexes[j];
           k < full_data_size - m_shared_data_object->indexes[j]; k++) {
        all_data[k + 1] = all_data[k];
      }
      for (k = 0; k < m_shared_data_object->index_table_size; k++) {
        char shared_data_part[((m_shared_data_object->data_loc.size +
                                SECTOR_SIZE) /
                               SECTOR_SIZE) *
                              SECTOR_SIZE];
        ret = !ssfs_read_dev(

            m_shared_data_object->data_loc.start_lba,
            (m_shared_data_object->data_loc.size + SECTOR_SIZE) / SECTOR_SIZE,
            shared_data_part);

        for (int l = m_shared_data_object->data_loc.start_byte;
             l < m_shared_data_object->data_loc.size; i++) {
          all_data[m_shared_data_object->indexes[j] + l] = shared_data_part[l];
        }
      }
    }
  }

  free(sh);
  free(m_final_buf);

  file_response resp = {.content = ret ? "\0" : all_data,
                        .size = ret ? 0 : full_data_size};

  // If 'ret' is NULL - return a null character, else return the data we just
  // read
  return resp;
}

// Delete a file or symlink
bool ssfs_del_file_or_link(char path[]) {
  bool ret;
  ssfs_filename_info info = ssfs_get_filename_info(path);

  char sh_buf[SECTOR_SIZE];
  ret = !ssfs_read_dev((uint32_t)get_part_start(), 1, sh_buf);

  ssfs_header *sh = malloc(sizeof(ssfs_header));
  memcpy(sh, sh_buf, sizeof(ssfs_header));

  if (sh->total_dirents == 0)
    return true;

  uint32_t dirent_table_size = sh->total_dirents == 0 ? 1 : sh->total_dirents;
  if (dirent_table_size == 0)
    return true;
  uint64_t dirent_loc = get_part_start() + 1;

  ssfs_dirent dirent_table[dirent_table_size];
  ret = !ssfs_read_dev(
      dirent_loc,
      ((sizeof(ssfs_dirent) * sh->total_dirents) + SECTOR_SIZE) / SECTOR_SIZE,
      dirent_table);

  ssfs_journal *j = read_journal();
  ssfs_dirent dirent = ssfs_get_dirent_from_info(info, dirent_table);

  if (dirent.type == 1)
    return true;

  ssfs_journal_orphaned_dirents *orphaned_dirents =
      malloc(j->orphaned_dirents_size + sizeof(ssfs_journal_orphaned_dirents) +
             sizeof(uint64_t));
  for (int i = 0; i < j->orphaned_dirents.orphaned_dirents; i++) {
    orphaned_dirents->orphaned_dirent_indexes[i] =
        j->orphaned_dirents.orphaned_dirent_indexes[i];
    orphaned_dirents->orphaned_dirents++;
  }

  for (int i = 0; i < dirent_table_size; i++) {
    if (ssfs_get_dirent_from_info(info, dirent_table)
                .parent_dirent_table_index ==
            dirent_table[i].parent_dirent_table_index &&
        memcmp(info.filename, dirent_table[i].name, info.filename_size)) {
      orphaned_dirents
          ->orphaned_dirent_indexes[j->orphaned_dirents.orphaned_dirents] = i;
      orphaned_dirents->orphaned_dirents++;
      break;
    }
  }

  reload_journal(&j->broken_data, orphaned_dirents, &j->cached_data,
                 j->broken_data_size,
                 j->orphaned_dirents_size +
                     sizeof(ssfs_journal_orphaned_dirents) + sizeof(uint64_t),
                 j->cached_data_size);

  free(orphaned_dirents);

  ssfs_dirent ret_dirent_table[dirent_table_size - 1];

  uint64_t i = 0;
  char
      m_dirent_inode_buf[((dirent.inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE) *
                         SECTOR_SIZE];
  ret = !ssfs_read_dev(dirent.inode_loc.start_lba,
                       (dirent.inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE,
                       m_dirent_inode_buf);

  char m_final_buf[(((dirent.inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE) *
                    SECTOR_SIZE) -
                   dirent.inode_loc.start_byte];
  uint32_t index = 0;
  for (i = dirent.inode_loc.start_byte;
       i < ((dirent.inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE) * SECTOR_SIZE;
       i++) {
    m_final_buf[index] = m_dirent_inode_buf[i];
    index++;
  }

  ssfs_inode *m_file_info = (ssfs_inode *)m_final_buf;

  for (i = 0; i < dirent_table_size - 1; i++) {
    if (dirent.type == 3)
      break;
    char dirent_inode_buf[((ret_dirent_table[i].inode_loc.size + SECTOR_SIZE) /
                           SECTOR_SIZE) *
                          SECTOR_SIZE];
    ret = !ssfs_read_dev(ret_dirent_table[i].inode_loc.start_lba,
                         (ret_dirent_table[i].inode_loc.size + SECTOR_SIZE) /
                             SECTOR_SIZE,
                         dirent_inode_buf);

    char final_buf[(((ret_dirent_table[i].inode_loc.size + SECTOR_SIZE) /
                     SECTOR_SIZE) *
                    SECTOR_SIZE) -
                   ret_dirent_table[i].inode_loc.start_byte];
    uint32_t i = 0;
    uint32_t index = 0;
    for (i = ret_dirent_table[i].inode_loc.start_byte;
         i <
         ((ret_dirent_table[i].inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE) *
             SECTOR_SIZE;
         i++) {
      final_buf[index] = dirent_inode_buf[i];
      index++;
    }

    ssfs_inode *file_info = (ssfs_inode *)final_buf;

    for (int j = 0; j < file_info->shared_data.shared_data_size; j++) {
      char shared_data_buf[((dirent.inode_loc.size + SECTOR_SIZE) /
                            SECTOR_SIZE) *
                           SECTOR_SIZE];

      ret = !ssfs_read_dev(

          file_info->shared_data.shared_data_locations[j].start_lba,
          (dirent.inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE, shared_data_buf);

      char final_buf[file_info->shared_data.shared_data_locations[j].size];
      int index = 0;
      for (int k = file_info->shared_data.shared_data_locations[j].start_byte;
           k < file_info->shared_data.shared_data_locations[j].start_byte +
                   file_info->shared_data.shared_data_locations[j].size;
           k++) {
        final_buf[index] = shared_data_buf[k];
        index++;
      }

      ssfs_shared_data_object *m_shared_data_object =
          (ssfs_shared_data_object *)final_buf;
      for (int k = 0; k < m_file_info->data_loc_size; k++) {
        if (m_shared_data_object->data_loc.start_lba ==
                m_file_info->data_loc_table[k].start_lba &&
            m_shared_data_object->data_loc.size ==
                m_file_info->data_loc_table[k].size &&
            m_shared_data_object->data_loc.start_byte ==
                m_file_info->data_loc_table[k].start_byte) {
          for (int l = 0; l < file_info->data_loc_size; i++) {
            char data[((file_info->data_loc_table[l].size + SECTOR_SIZE) /
                       SECTOR_SIZE) *
                      SECTOR_SIZE];
            ret = !ssfs_read_dev(

                file_info->data_loc_table[l].start_lba,
                (file_info->data_loc_table[l].size + SECTOR_SIZE) / SECTOR_SIZE,
                data);
          }
        }
      }
    }
  }
  uint64_t free_blocks_table_loc =
      get_part_start() +
      (((sizeof(ssfs_dirent) * dirent_table_size) + SECTOR_SIZE) /
       SECTOR_SIZE) +
      1;

  for (i = 0; i < m_file_info->data_loc_size; i++) {
    if (dirent.type == 3)
      break;

    reload_header(
        0, sh->total_dirents - 1, sh->data_used - m_file_info->data_loc_size,
        sh->free_block_table_size +
            (m_file_info->data_loc_table[i].size + SECTOR_SIZE) / SECTOR_SIZE,
        sh->free_dirent_table_size, sh->free_inode_table_size,
        sh->dirent_inode_table_byte_size - dirent.inode_loc.size,
        sh->inode_shared_data_table_byte_size, sh->write_backwards);

    if (((m_file_info->data_loc_table[i].size + SECTOR_SIZE) / SECTOR_SIZE) ==
        0) {
      uint64_t free_blocks_table[(
          (m_file_info->data_loc_table[i].size + SECTOR_SIZE) / SECTOR_SIZE)];
      uint64_t k = 0;

      for (uint64_t j = m_file_info->data_loc_table[i].start_lba;
           j < m_file_info->data_loc_table[i].start_lba +
                   m_file_info->data_loc_table[i].size;
           j++) {
        free_blocks_table[k] = j;
        k++;
      }
      ret = !ssfs_write_dev(
          free_blocks_table_loc,
          (m_file_info->data_loc_table[i].size + SECTOR_SIZE) / SECTOR_SIZE,
          free_blocks_table);

      continue;
    }

    uint64_t
        temp_blocks_table[(m_file_info->data_loc_table[i].size + SECTOR_SIZE) /
                          SECTOR_SIZE];
    uint32_t j, k;

    for (j = m_file_info->data_loc_table[i].start_lba, k = 0;
         j < m_file_info->data_loc_table[i].size; i++, k++) {
      temp_blocks_table[i] = j;
    }

    uint64_t free_blocks_table
        [((m_file_info->data_loc_table[i].size + SECTOR_SIZE) / SECTOR_SIZE) +
         (sizeof(temp_blocks_table) / sizeof(temp_blocks_table[0]))];
    ret = !ssfs_read_dev(free_blocks_table_loc,
                         (m_file_info->data_loc_table[i].size + SECTOR_SIZE) /
                             SECTOR_SIZE,
                         free_blocks_table);

    for (i = (m_file_info->data_loc_table[i].size + SECTOR_SIZE) / SECTOR_SIZE,
        j = 0;
         i < sizeof(temp_blocks_table) / sizeof(temp_blocks_table[0]);
         i++, j++) {
      free_blocks_table[i] = temp_blocks_table[j];
    }
    ret = !ssfs_write_dev(free_blocks_table_loc,
                          (m_file_info->data_loc_table[i].size + SECTOR_SIZE) /
                              SECTOR_SIZE,
                          free_blocks_table);
  }
  ret = !ssfs_write_dev(
      dirent_loc,
      ((sizeof(ssfs_dirent) * (sh->total_dirents - 1)) + SECTOR_SIZE) /
          SECTOR_SIZE,
      ret_dirent_table);

  uint64_t inode_loc =
      get_part_end() - sh->journal_size -
      ((sh->dirent_inode_table_byte_size + SECTOR_SIZE) / SECTOR_SIZE) - 1;

  char dirent_inode_buf[((sh->dirent_inode_table_byte_size + SECTOR_SIZE) /
                         SECTOR_SIZE) *
                        SECTOR_SIZE];
  ret = !ssfs_read_dev(
      inode_loc, (sh->dirent_inode_table_byte_size + SECTOR_SIZE) / SECTOR_SIZE,
      dirent_inode_buf);

  char final_inode_buf[(((sh->dirent_inode_table_byte_size + SECTOR_SIZE) /
                         SECTOR_SIZE) *
                        SECTOR_SIZE) -
                       dirent.inode_loc.size];
  for (i = 0;
       i < ((sh->dirent_inode_table_byte_size + SECTOR_SIZE) / SECTOR_SIZE) *
               SECTOR_SIZE;
       i++) {
    if (i >= dirent.inode_loc.start_lba + dirent.inode_loc.start_byte &&
        i <= dirent.inode_loc.size)
      continue;
    final_inode_buf[i] = dirent_inode_buf[i];
  }

  ret = !ssfs_write_dev(
      inode_loc,
      ((((sh->dirent_inode_table_byte_size + SECTOR_SIZE) / SECTOR_SIZE) *
        SECTOR_SIZE) -
       dirent.inode_loc.size) /
          SECTOR_SIZE,
      final_inode_buf);

  reload_journal(&j->broken_data, &j->orphaned_dirents, &j->cached_data,
                 j->broken_data_size, j->orphaned_dirents_size,
                 j->cached_data_size);

  free(sh);
  free(j);

  return ret;
}

bool ssfs_del_dir(char path[]) {
  bool ret;
  ssfs_filename_info info = ssfs_get_filename_info(path);

  char sh_buf[SECTOR_SIZE];
  ret = !ssfs_read_dev((uint32_t)get_part_start(), 1, sh_buf);

  ssfs_header *sh = malloc(sizeof(ssfs_header));
  memcpy(sh, sh_buf, sizeof(ssfs_header));

  uint32_t dirent_table_size = sh->total_dirents == 0 ? 1 : sh->total_dirents;
  uint64_t dirent_loc = get_part_start() + 1;

  ssfs_dirent dirent_table[dirent_table_size];
  ret = !ssfs_read_dev(
      dirent_loc,
      ((sizeof(ssfs_dirent) * (sh->total_dirents + 1)) + SECTOR_SIZE) /
          SECTOR_SIZE,
      dirent_table);

  ssfs_journal *j = read_journal();
  ssfs_dirent dirent = ssfs_get_dirent_from_info(info, dirent_table);

  if (dirent.type != 1)
    return true;

  uint32_t dir_index = 0;
  uint32_t child_dirent_table_size = 0;
  uint32_t i = 0;

  for (i = 0; i < dirent_table_size; i++) {
    if (memcmp(dirent_table[i].name, info.filename,
               strlen(dirent_table[i].name)) &&
        dirent_table[i].parent_dirent_table_index ==
            dirent.parent_dirent_table_index) {
      dir_index = i;
      break;
    }
  }

  for (i = 0; i < dirent_table_size; i++) {
    if (dirent_table[i].parent_dirent_table_index == dir_index)
      child_dirent_table_size++;
  }

  ssfs_journal_orphaned_dirents *orphaned_dirents =
      malloc(j->orphaned_dirents_size + sizeof(ssfs_journal_orphaned_dirents) +
             (sizeof(uint64_t) * child_dirent_table_size));
  for (int i = 0; i < j->orphaned_dirents.orphaned_dirents; i++) {
    orphaned_dirents->orphaned_dirent_indexes[i] =
        j->orphaned_dirents.orphaned_dirent_indexes[i];
    orphaned_dirents->orphaned_dirents++;
  }

  for (int i = 0; i < dirent_table_size; i++) {
    if (ssfs_get_dirent_from_info(info, dirent_table)
                .parent_dirent_table_index ==
            dirent_table[i].parent_dirent_table_index &&
        memcmp(info.filename, dirent_table[i].name, info.filename_size)) {
      orphaned_dirents
          ->orphaned_dirent_indexes[j->orphaned_dirents.orphaned_dirents] = i;
      orphaned_dirents->orphaned_dirents++;
      break;
    }
  }

  reload_journal(
      &j->broken_data, orphaned_dirents, &j->cached_data, j->broken_data_size,
      j->orphaned_dirents_size + sizeof(ssfs_journal_orphaned_dirents) +
          (sizeof(uint64_t) * child_dirent_table_size),
      j->cached_data_size);
  free(orphaned_dirents);

  uint32_t child_dirent_table_indexes[child_dirent_table_size];
  ssfs_dirent ret_dirent_table[dirent_table_size - child_dirent_table_size];

  uint32_t index = 0;
  for (i = 0; i < dirent_table_size; i++) {
    if (dirent_table[i].parent_dirent_table_index == dir_index) {
      child_dirent_table_indexes[index] = i;
      index++;
    }
  }

  for (i = 0; i < child_dirent_table_size; i++) {
    orphaned_dirents
        ->orphaned_dirent_indexes[j->orphaned_dirents.orphaned_dirents] =
        child_dirent_table_indexes[i];
    orphaned_dirents->orphaned_dirents++;
  }

  for (i = 0; i < dirent_table_size; i++) {
    for (int k = 0; k < child_dirent_table_size; k++) {
      char dir_path[strlen(path) + dirent.name_size + 1];
      strcat(dir_path, path);
      strcat(dir_path, "/");

      uint32_t index_dir_path = 0;
      for (int k = strlen(path) + 1; k < dirent.name_size + strlen(path) + 1;
           k++) {
        dir_path[k] = dirent_table[i].name[index_dir_path];
        index_dir_path++;
      }

      if (dirent_table[i].type == 1) {
        ret = ssfs_del_dir(dir_path);
        continue;
      }
      ssfs_del_file_or_link(dir_path);
    }
  }

  ret = !ssfs_write_dev(
      dirent_loc,
      ((sizeof(ssfs_dirent) * (sh->total_dirents - 1)) + SECTOR_SIZE) /
          SECTOR_SIZE,
      ret_dirent_table);

  reload_journal(&j->broken_data, &j->orphaned_dirents, &j->cached_data,
                 j->broken_data_size, j->orphaned_dirents_size,
                 j->cached_data_size);

  free(sh);
  free(j);

  return ret;
}

bool ssfs_mk_symlink(char src_path[], char target_path[]) {
  bool ret;
  ssfs_filename_info info = ssfs_get_filename_info(target_path);

  char sh_buf[SECTOR_SIZE];
  ret = !ssfs_read_dev((uint32_t)get_part_start(), 1, sh_buf);

  ssfs_header *sh = malloc(sizeof(ssfs_header));
  memcpy(sh, sh_buf, sizeof(ssfs_header));

  uint32_t dirent_table_size = sh->total_dirents + 1;
  uint64_t dirent_loc = get_part_start() + 1;

  ssfs_dirent dirent_buf[dirent_table_size];
  if (dirent_table_size > 0)
    ret = !ssfs_read_dev(
        dirent_loc,
        ((sizeof(ssfs_dirent) * (sh->total_dirents + 1)) + SECTOR_SIZE) /
            SECTOR_SIZE,
        dirent_buf);
  ssfs_dirent new_dirent;
  memset(&new_dirent, 0, sizeof(new_dirent));

  ssfs_journal *j = read_journal();
  ssfs_dirent dirent = ssfs_get_dirent_from_info(info, dirent_buf);

  if (dirent.type == 3)
    return true;

  ssfs_journal_orphaned_dirents *orphaned_dirents =
      malloc(j->orphaned_dirents_size + sizeof(ssfs_journal_orphaned_dirents) +
             sizeof(uint64_t));
  for (int i = 0; i < j->orphaned_dirents.orphaned_dirents; i++) {
    orphaned_dirents->orphaned_dirent_indexes[i] =
        j->orphaned_dirents.orphaned_dirent_indexes[i];
    orphaned_dirents->orphaned_dirents++;
  }

  for (int i = 0; i < dirent_table_size; i++) {
    if (ssfs_get_dirent_from_info(ssfs_get_filename_info(target_path),
                                  dirent_buf)
                .parent_dirent_table_index ==
            dirent_buf[i].parent_dirent_table_index &&
        memcmp(ssfs_get_filename_info(target_path).filename, dirent_buf[i].name,
               ssfs_get_filename_info(target_path).filename_size)) {
      orphaned_dirents
          ->orphaned_dirent_indexes[j->orphaned_dirents.orphaned_dirents] = i;
      orphaned_dirents->orphaned_dirents++;
      break;
    }
  }
  reload_journal(&j->broken_data, orphaned_dirents, &j->cached_data,
                 j->broken_data_size,
                 j->orphaned_dirents_size +
                     sizeof(ssfs_journal_orphaned_dirents) + sizeof(uint64_t),
                 j->cached_data_size);
  free(orphaned_dirents);

  memcpy(dirent_buf,
         ssfs_setup_dirent_parents(ssfs_get_filename_info(src_path), new_dirent,
                                   dirent_buf)
             .dirent_table,
         dirent_table_size * sizeof(ssfs_dirent));

  ssfs_dirent last_dirent =
      ssfs_setup_dirent_parents(ssfs_get_filename_info(src_path), new_dirent,
                                dirent_buf)
          .last_child;
  new_dirent = last_dirent;

  memcpy(new_dirent.name, ssfs_get_filename_info(src_path).filename,
         ssfs_get_filename_info(src_path).filename_size);
  new_dirent.name_size = strlen(ssfs_get_filename_info(src_path).filename);

  new_dirent.type = 3;
  new_dirent.inode_loc.start_lba = -1;
  new_dirent.inode_loc.size = 0;
  new_dirent.inode_loc.start_byte = -1;

  for (int i = 0; i < dirent_table_size; i++) {
    if (ssfs_get_dirent_from_info(info, dirent_buf).parent_dirent_table_index ==
            dirent_buf[i].parent_dirent_table_index &&
        memcmp(info.filename, dirent_buf[i].name, info.filename_size)) {
      new_dirent.is_link = true;
      new_dirent.link_type = 0x1;
      new_dirent.link_dirent_table_index = i;
      break;
    }
  }
  dirent_buf[dirent_table_size - 1] = new_dirent;

  reload_header(0, sh->total_dirents + 1, sh->data_used,
                sh->free_block_table_size, sh->free_dirent_table_size,
                sh->free_inode_table_size, sh->dirent_inode_table_byte_size,
                sh->inode_shared_data_table_byte_size, sh->write_backwards);

  ret = !ssfs_write_dev(
      dirent_loc,
      ((sizeof(ssfs_dirent) * (sh->total_dirents + 1)) + SECTOR_SIZE) /
          SECTOR_SIZE,
      dirent_buf);

  reload_journal(&j->broken_data, &j->orphaned_dirents, &j->cached_data,
                 j->broken_data_size, j->orphaned_dirents_size,
                 j->cached_data_size);
  free(sh);
  free(j);
  return ret;
}

bool ssfs_mk_dir(char path[], uint16_t permissions) {
  bool ret;
  ssfs_filename_info info = ssfs_get_filename_info(path);

  char sh_buf[SECTOR_SIZE];
  ret = !ssfs_read_dev((uint32_t)get_part_start(), 1, sh_buf);

  ssfs_header *sh = malloc(sizeof(ssfs_header));
  memcpy(sh, sh_buf, sizeof(ssfs_header));

  uint32_t dirent_table_size = sh->total_dirents + 1;
  uint64_t dirent_loc = get_part_start() + 1;
  
  ssfs_dirent dirent_buf[dirent_table_size];
  if (dirent_table_size > 1)
    ret = !ssfs_read_dev(
        dirent_loc,
        ((sizeof(ssfs_dirent) * (sh->total_dirents + 1)) + SECTOR_SIZE) /
            SECTOR_SIZE,
        dirent_buf);
  ssfs_dirent new_dirent;
  memset(&new_dirent, 0, sizeof(new_dirent));

  ssfs_journal *j = read_journal();
  ssfs_dirent dirent = ssfs_get_dirent_from_info(info, dirent_buf);

  if (dirent.type != 1)
    return true;

  ssfs_journal_orphaned_dirents *orphaned_dirents =
      malloc(j->orphaned_dirents_size + sizeof(ssfs_journal_orphaned_dirents) +
             sizeof(uint64_t));
  for (int i = 0; i < j->orphaned_dirents.orphaned_dirents; i++) {
    orphaned_dirents->orphaned_dirent_indexes[i] =
        j->orphaned_dirents.orphaned_dirent_indexes[i];
    orphaned_dirents->orphaned_dirents++;
  }

  for (int i = 0; i < dirent_table_size; i++) {
    if (ssfs_get_dirent_from_info(info, dirent_buf).parent_dirent_table_index ==
            dirent_buf[i].parent_dirent_table_index &&
        memcmp(info.filename, dirent_buf[i].name, info.filename_size)) {
      orphaned_dirents
          ->orphaned_dirent_indexes[j->orphaned_dirents.orphaned_dirents] = i;
      orphaned_dirents->orphaned_dirents++;
      break;
    }
  }

  ssfs_journal_broken_data *broken_data =
      malloc(j->broken_data_size + sizeof(ssfs_journal_broken_data) +
             sizeof(ssfs_dirent));
  for (int i = 0; i < j->broken_data.broken_dirents; i++) {
    broken_data->broken_dirent_table[i] = j->broken_data.broken_dirent_table[i];
    broken_data->broken_dirents++;
  }

  broken_data->broken_dirent_table[j->broken_data.broken_dirents] = new_dirent;
  broken_data->broken_dirents++;

  reload_journal(broken_data, orphaned_dirents, &j->cached_data,
                 j->broken_data_size + sizeof(ssfs_journal_broken_data) +
                     sizeof(ssfs_dirent),
                 j->orphaned_dirents_size +
                     sizeof(ssfs_journal_orphaned_dirents) + sizeof(uint64_t),
                 j->cached_data_size);
  free(broken_data);
  free(orphaned_dirents);

  memcpy(dirent_buf,
         ssfs_setup_dirent_parents(info, new_dirent, dirent_buf).dirent_table,
         dirent_table_size * sizeof(ssfs_dirent));

  ssfs_dirent last_dirent =
      ssfs_setup_dirent_parents(info, new_dirent, dirent_buf).last_child;
  new_dirent = last_dirent;

  memcpy(new_dirent.name, ssfs_get_filename_info(path).filename,
         ssfs_get_filename_info(path).filename_size);
  new_dirent.name_size = strlen(ssfs_get_filename_info(path).filename);

  new_dirent.permissions = permissions;
  new_dirent.type = 1;
  new_dirent.inode_loc.start_lba = -1;
  new_dirent.inode_loc.size = 0;
  new_dirent.inode_loc.start_byte = -1;

  dirent_buf[dirent_table_size] = new_dirent;

  reload_header(0, sh->total_dirents + 1, sh->data_used,
                sh->free_block_table_size, sh->free_dirent_table_size,
                sh->free_inode_table_size, sh->dirent_inode_table_byte_size,
                sh->inode_shared_data_table_byte_size, sh->write_backwards);

  ret = !ssfs_write_dev(
      dirent_loc,
      ((sizeof(ssfs_dirent) * (sh->total_dirents + 1)) + SECTOR_SIZE) /
          SECTOR_SIZE,
      dirent_buf);

  reload_journal(&j->broken_data, &j->orphaned_dirents, &j->cached_data,
                 j->broken_data_size, j->orphaned_dirents_size,
                 j->cached_data_size);
  free(sh);
  free(j);

  return ret;
}

bool ssfs_mk_file(char path[], char data[], uint16_t permissions) {
  bool ret;
  uint32_t data_size = strlen(data);

  char sh_buf[SECTOR_SIZE];
  ret = !ssfs_read_dev(get_part_start(), 1, sh_buf);

  ssfs_header *sh = malloc(sizeof(ssfs_header));
  memcpy(sh, sh_buf, sizeof(ssfs_header));

  ssfs_dirent new_dirent;

  uint32_t dirent_table_size = sh->total_dirents + 1;
  uint64_t free_blocks_table_loc =
      get_part_start() +
      (((sizeof(ssfs_dirent) * dirent_table_size) + SECTOR_SIZE) /
       SECTOR_SIZE) +
      1;

  uint64_t shared_data_loc =
      get_part_end() -
      ((sh->dirent_inode_table_byte_size +
        sh->inode_shared_data_table_byte_size + SECTOR_SIZE) /
       SECTOR_SIZE) -
      1;

  uint64_t dirent_loc = get_part_start() + 1;
  ssfs_dirent dirent_buf[dirent_table_size];

  if (dirent_table_size > 1)
    ret = !ssfs_read_dev(
        dirent_loc,
        ((sizeof(ssfs_dirent) * (sh->total_dirents + 1)) + SECTOR_SIZE) /
            SECTOR_SIZE,
        dirent_buf);

  uint64_t inode_size = 0;
  if (sh->free_block_table_size == 0)
    inode_size = sizeof(ssfs_inode) + sizeof(ssfs_data_loc) +
                 (sh->total_dirents == 0
                      ? 0
                      : (ssfs_get_dirent_shared_data(data, dirent_buf, 0)
                             .shared_data_len *
                         sizeof(ssfs_data_loc)));
  else {
    uint32_t data_shared_size = 0;
    uint32_t free_size = 0;
    for (int i = 0; i < sh->free_block_table_size; i++) {
      data_shared_size +=
          ssfs_get_dirent_shared_data(data, dirent_buf, i).shared_data_size;
    }
    for (int i = 0; i < sh->free_block_table_size; i++) {
      if (data_size > SECTOR_SIZE &&
          (data_size + SECTOR_SIZE) / SECTOR_SIZE > i)
        break;
      free_size++;
    }
    inode_size = sizeof(ssfs_inode) + (sizeof(ssfs_data_loc) * free_size) +
                 (data_shared_size * sizeof(ssfs_data_loc));
  }

  char info[inode_size];
  ssfs_inode *new_dirent_info = (ssfs_inode *)info;

  uint64_t inode_loc =
      get_part_end() - sh->journal_size -
      ((sh->dirent_inode_table_byte_size + inode_size + SECTOR_SIZE) /
       SECTOR_SIZE) -
      1;

  char dirent_inode_buf[sh->dirent_inode_table_byte_size + inode_size];
  if (dirent_table_size > 1)
    ret = !ssfs_read_dev(inode_loc,
                         (sh->dirent_inode_table_byte_size + SECTOR_SIZE) /
                             SECTOR_SIZE,
                         dirent_inode_buf);

  char inode_shared_data_buf[sh->inode_shared_data_table_byte_size +
                             ssfs_get_dirent_shared_data(data, dirent_buf, 0)
                                 .shared_data_size];
  if (dirent_table_size > 1)
    ret = !ssfs_read_dev(shared_data_loc,
                         (sh->inode_shared_data_table_byte_size + SECTOR_SIZE) /
                             SECTOR_SIZE,
                         inode_shared_data_buf);

  memcpy(new_dirent.name, ssfs_get_filename_info(path).filename,
         ssfs_get_filename_info(path).filename_size);
  new_dirent.name_size = ssfs_get_filename_info(path).filename_size;

  new_dirent.type = 2;
  new_dirent.permissions = permissions;

  new_dirent_info->shared_data.shared_data_size =
      ssfs_get_dirent_shared_data(data, dirent_buf, 0).shared_data_size;

  new_dirent_info->data_loc_table[0].size =
      ssfs_get_dirent_shared_data(data, dirent_buf, 0).updated_raw_data_size ==
              0
          ? data_size
          : ssfs_get_dirent_shared_data(data, dirent_buf, 0)
                .updated_raw_data_size;

  uint64_t data_loc = ((get_part_end() - get_part_start()) / 2) +
                      (((sh->data_used + SECTOR_SIZE) / SECTOR_SIZE) / 2);

  if (sh->write_backwards)
    data_loc = data_loc - (((sh->data_used + SECTOR_SIZE) / SECTOR_SIZE) / 2) -
               ((data_size + SECTOR_SIZE) / SECTOR_SIZE);

  new_dirent_info->data_loc_table[0].start_lba = data_loc;
  new_dirent_info->data_loc_size = 1;

  uint64_t free_blocks_table[sh->free_block_table_size];
  if (sh->free_block_table_size > 0)
    ret =
        !ssfs_read_dev(free_blocks_table_loc,
                       (sh->free_block_table_size + SECTOR_SIZE) / SECTOR_SIZE,
                       free_blocks_table);

  uint64_t overlapping_blocks =
      ((data_size + SECTOR_SIZE) / SECTOR_SIZE) > sh->free_block_table_size
          ? sh->free_block_table_size -
                ((data_size + SECTOR_SIZE) / SECTOR_SIZE)
          : 0;
  for (int i = 0; i < sh->free_block_table_size; i++) {
    if (data_size > SECTOR_SIZE && (data_size + SECTOR_SIZE) / SECTOR_SIZE > i)
      break;

    new_dirent_info->data_loc_table[i].start_lba = free_blocks_table[i];

    if (data_size <= SECTOR_SIZE)
      new_dirent_info->data_loc_table[i].size = data_size;
    else
      new_dirent_info->data_loc_table[i].size = SECTOR_SIZE;
    new_dirent_info->data_loc_size += 1;
  }
  new_dirent.parent_dirent_table_index = 0;

  ssfs_journal *j = read_journal();
  ssfs_journal_orphaned_dirents *orphaned_dirents =
      malloc(j->orphaned_dirents_size + sizeof(ssfs_journal_orphaned_dirents) +
             sizeof(uint64_t));

  for (int i = 0; i < j->orphaned_dirents.orphaned_dirents; i++) {
    orphaned_dirents->orphaned_dirent_indexes[i] =
        j->orphaned_dirents.orphaned_dirent_indexes[i];
    orphaned_dirents->orphaned_dirents++;
  }

  orphaned_dirents
      ->orphaned_dirent_indexes[j->orphaned_dirents.orphaned_dirents] =
      sh->total_dirents;
  orphaned_dirents->orphaned_dirents++;

  ssfs_journal_broken_data *broken_data =
      malloc(j->broken_data_size + sizeof(ssfs_journal_broken_data) +
             sizeof(ssfs_dirent));
  for (int i = 0; i < j->broken_data.broken_dirents; i++) {
    broken_data->broken_dirent_table[i] = j->broken_data.broken_dirent_table[i];
    broken_data->broken_dirents++;
  }
  broken_data->broken_dirent_table[j->broken_data.broken_dirents] = new_dirent;
  broken_data->broken_dirents++;

  ssfs_journal_cached_data *cached_data =
      malloc(j->cached_data_size + sizeof(ssfs_journal_cached_data) +
             (sizeof(ssfs_data_loc) * new_dirent_info->data_loc_size));
  for (uint64_t i = 0; i < j->cached_data.cached_data_size; i++) {
    for (uint64_t j__ = 0; j__ < new_dirent_info->data_loc_size; j__++) {
      cached_data->data_to_be_written[i + j__] =
          new_dirent_info->data_loc_table[j__];
      cached_data->cached_data_size++;
    }
  }

  for (int i = 0; i < new_dirent_info->data_loc_size; i++) {
    cached_data->data_to_be_written[j->broken_data.broken_dirents + i] =
        new_dirent_info->data_loc_table[i];
    cached_data->cached_data_size++;
  }

  reload_journal(broken_data, orphaned_dirents, cached_data,
                 j->broken_data_size + sizeof(ssfs_journal_broken_data) +
                     sizeof(ssfs_dirent),
                 j->orphaned_dirents_size +
                     sizeof(ssfs_journal_orphaned_dirents) + sizeof(uint64_t),
                 j->cached_data_size + sizeof(ssfs_journal_cached_data) +
                     (sizeof(ssfs_data_loc) * new_dirent_info->data_loc_size));

  new_dirent.inode_loc.size = inode_size;
  new_dirent.inode_loc.start_byte =
      sh->dirent_inode_table_byte_size % SECTOR_SIZE;

  int shared_data_size = sh->inode_shared_data_table_byte_size;
  for (int i = 0;
       i < ssfs_get_dirent_shared_data(data, dirent_buf, 0).shared_data_len;
       i++) {
    if (i == 0) {
      new_dirent_info->shared_data.shared_data_locations[i].start_byte =
          sh->inode_shared_data_table_byte_size == 0 ||
                  sh->inode_shared_data_table_byte_size < SECTOR_SIZE
              ? 0
              : sh->inode_shared_data_table_byte_size % SECTOR_SIZE;
      sh->inode_shared_data_table_byte_size +=
          sizeof(ssfs_shared_data_object) +
          (ssfs_get_dirent_shared_data(data, dirent_buf, 0)
               .shared_data[i]
               ->index_table_size *
           sizeof(uint64_t));
      continue;
    }

    new_dirent_info->shared_data.shared_data_locations[i].start_lba =
        (sh->inode_shared_data_table_byte_size + SECTOR_SIZE) / SECTOR_SIZE;
    sh->inode_shared_data_table_byte_size +=
        sizeof(ssfs_shared_data_object) +
        (ssfs_get_dirent_shared_data(data, dirent_buf, 0)
             .shared_data[i]
             ->index_table_size *
         sizeof(uint64_t));

    new_dirent_info->shared_data.shared_data_locations[i].start_byte =
        sh->inode_shared_data_table_byte_size % SECTOR_SIZE;
    new_dirent_info->shared_data.shared_data_locations[i].size =
        sizeof(ssfs_shared_data_object) +
        (ssfs_get_dirent_shared_data(data, dirent_buf, 0)
             .shared_data[i]
             ->index_table_size *
         sizeof(uint64_t));
  }
  ret = !ssfs_read_dev(get_part_start(), 1, sh_buf);
  memcpy(sh, sh_buf, sizeof(ssfs_header));

  inode_loc = get_part_end() - sh->journal_size -
              ((sh->dirent_inode_table_byte_size + inode_size + SECTOR_SIZE) /
               SECTOR_SIZE) -
              1;

  new_dirent.inode_loc.start_lba =
      sh->dirent_inode_table_byte_size == 0 ||
              sh->dirent_inode_table_byte_size < SECTOR_SIZE
          ? inode_loc
          : inode_loc +
                ((sh->dirent_inode_table_byte_size + SECTOR_SIZE) /
                 SECTOR_SIZE) -
                1;

  memcpy(dirent_buf,
         ssfs_setup_dirent_parents(ssfs_get_filename_info(path), new_dirent,
                                   dirent_buf)
             .dirent_table,
         dirent_table_size * sizeof(ssfs_dirent));

  dirent_buf[dirent_table_size - 1] =
      ssfs_setup_dirent_parents(ssfs_get_filename_info(path), new_dirent,
                                dirent_buf)
          .last_child;

  ret = !ssfs_write_dev(
      dirent_loc,
      ((sizeof(ssfs_dirent) * (sh->total_dirents + 1)) + SECTOR_SIZE) /
          SECTOR_SIZE,
      dirent_buf);

  uint32_t index = 0;
  for (int i = sh->dirent_inode_table_byte_size; i < inode_size; i++) {
    dirent_inode_buf[i] = ((char *)new_dirent_info)[index];
    index++;
  }

  index = 0;
  for (int i = shared_data_size; i < sh->inode_shared_data_table_byte_size;
       i++) {
    inode_shared_data_buf[i] =
        ((char *)ssfs_get_dirent_shared_data(data, dirent_buf, 0)
             .shared_data[ssfs_get_dirent_shared_data(data, dirent_buf, 0)
                              .shared_data_len -
                          1])[index];
    index++;
  }

  ret = !ssfs_write_dev(
      inode_loc, (sh->dirent_inode_table_byte_size + SECTOR_SIZE) / SECTOR_SIZE,
      dirent_inode_buf);

  if (dirent_table_size > 1)
    ret = !ssfs_write_dev(
        shared_data_loc,
        (sh->inode_shared_data_table_byte_size + SECTOR_SIZE) / SECTOR_SIZE,
        inode_shared_data_buf);

  if (sh->free_block_table_size == 0)
    ret = !ssfs_write_dev(data_loc, (data_size + SECTOR_SIZE) / SECTOR_SIZE,
                          ssfs_get_dirent_shared_data(data, dirent_buf, 0)
                                      .updated_raw_data_size == 0
                              ? data
                              : ssfs_get_dirent_shared_data(data, dirent_buf, 0)
                                    .updated_raw_data);

  else {
    uint64_t data_in_free_blocks_size = 0;
    for (int i = 0; i < new_dirent_info->data_loc_size; i++) {
      char split_data[new_dirent_info->data_loc_table[i].size];
      memcpy(split_data, data, new_dirent_info->data_loc_table[i].size);

      data_in_free_blocks_size += new_dirent_info->data_loc_table[i].size;

      ret = !ssfs_write_dev(

          new_dirent_info->data_loc_table[i].start_lba,
          (ssfs_get_dirent_shared_data(data, dirent_buf,
                                       new_dirent_info->data_loc_size)
               .updated_raw_data_size +
           SECTOR_SIZE) /
              SECTOR_SIZE,
          ssfs_get_dirent_shared_data(split_data, dirent_buf, i)
                      .updated_raw_data_size == 0
              ? data
              : ssfs_get_dirent_shared_data(split_data, dirent_buf, i)
                    .updated_raw_data);
    }

    if (overlapping_blocks > 0) {
      char remaining_data[(data_size - data_in_free_blocks_size + SECTOR_SIZE) /
                          SECTOR_SIZE];
      uint32_t i, j;

      for (i = data_size - data_in_free_blocks_size, j = 0;
           i < data_in_free_blocks_size; i++, j++) {
        remaining_data[j] = data[i];
      }

      ret = !ssfs_write_dev(
          data_loc,
          (ssfs_get_dirent_shared_data(remaining_data, dirent_buf,
                                       new_dirent_info->data_loc_size)
               .updated_raw_data_size +
           SECTOR_SIZE) /
              SECTOR_SIZE,
          ssfs_get_dirent_shared_data(remaining_data, dirent_buf,
                                      new_dirent_info->data_loc_size)
                      .updated_raw_data_size == 0
              ? data
              : ssfs_get_dirent_shared_data(remaining_data, dirent_buf,
                                            new_dirent_info->data_loc_size)
                    .updated_raw_data);
    }
  }

  uint64_t new_free_blocks_tbl[sh->free_block_table_size -
                               new_dirent_info->data_loc_size];
  if (sh->free_block_table_size > 0) {
    for (int i = 0;
         i < sh->free_block_table_size - new_dirent_info->data_loc_size; i++) {
      bool do_continue = false;
      for (int j__ = 0; j__ < new_dirent_info->data_loc_size; j__++) {
        if (new_dirent_info->data_loc_table[j__].start_lba ==
            free_blocks_table[i]) {
          do_continue = true;
          break;
        }
        for (int k = 0; k < index + 1; k++) {
          if (new_dirent_info->data_loc_table[j__].start_lba ==
              new_free_blocks_tbl[k]) {
            do_continue = true;
            break;
          }
        }
      }
      if (do_continue)
        continue;

      new_free_blocks_tbl[index] = free_blocks_table[i];
      index++;
    }

    ret = reload_header(
        sh->journal_size, sh->total_dirents, sh->data_used,
        sh->free_block_table_size - new_dirent_info->data_loc_size,
        sh->free_dirent_table_size, sh->free_inode_table_size,
        sh->dirent_inode_table_byte_size, sh->inode_shared_data_table_byte_size,
        sh->write_backwards);

    ret = !ssfs_write_dev(
        free_blocks_table_loc,
        ((sh->free_block_table_size - new_dirent_info->data_loc_size) +
         SECTOR_SIZE) /
            SECTOR_SIZE,
        new_free_blocks_tbl);
  }

  ret = reload_header(
      sh->journal_size, sh->total_dirents + 1, sh->data_used + data_size,
      sh->free_block_table_size, sh->free_dirent_table_size,
      sh->free_inode_table_size, sh->dirent_inode_table_byte_size + inode_size,
      sh->inode_shared_data_table_byte_size, !sh->write_backwards);
  ret = reload_journal(&j->broken_data, &j->orphaned_dirents, &j->cached_data,
                       j->broken_data_size, j->orphaned_dirents_size,
                       j->cached_data_size);

  free(new_dirent_info);
  free(sh);
  free(j);

  return ret;
}

extern bool tmpfs;
int device_handle = -1;

void ssfs_reload_driver(char *device) {
  close(device_handle);
  device_handle = open(device, 0, (char *)"r+");

  if (tmpfs) {
    reload_header(0, 0, 0, 0, 0, 0, 0, 0, false);

    ssfs_journal_orphaned_dirents orphaned_dirents;
    ssfs_journal_broken_data broken_data;
    ssfs_journal_cached_data cached_data;

    orphaned_dirents.orphaned_dirents = 0;
    broken_data.broken_dirents = 0;
    cached_data.cached_data_size = 0;

    reload_journal(&broken_data, &orphaned_dirents, &cached_data,
                   sizeof(ssfs_journal_broken_data),
                   sizeof(ssfs_journal_orphaned_dirents),
                   sizeof(ssfs_journal_cached_data));
  }
}