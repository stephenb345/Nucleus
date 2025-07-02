// stat.c - POSIX 'stat' system call
// Created 2024/11/29 by Stephen Byrne

#include "stat.h"

int stat(const char *path, s_stat *st) {
  st->st_uid = 0;
  st->st_gid = 0;
  st->st_atime = 0;
  st->st_ctime = 0;

  char sh_buf[SECTOR_SIZE];
  if (!ssfs_read_dev(0, 1, sh_buf))
    return -1;

  ssfs_header *sh = malloc(sizeof(ssfs_header));
  memcpy(sh, sh_buf, sizeof(ssfs_header));

  uint32_t dirent_table_size = sh->total_dirents + 1;

  // Read the SSFS dirent table
  ssfs_dirent dirent_table[dirent_table_size];
  if (ssfs_read_dev(1,
                    ((sizeof(ssfs_dirent) * sh->total_dirents) + SECTOR_SIZE) /
                        SECTOR_SIZE,
                    dirent_table))
    return -1;

  free(sh);

  ssfs_dirent dirent =
      ssfs_get_dirent_from_info(ssfs_get_filename_info((char*)path), dirent_table);

  uint64_t i = 0;
  char
      m_dirent_inode_buf[((dirent.inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE) *
                         SECTOR_SIZE];
  if (ssfs_read_dev(dirent.inode_loc.start_lba,
                    (dirent.inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE,
                    m_dirent_inode_buf))
    return -1;

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

  if (strcmp(path, "/") == 0) {
    st->st_mode = S_IFDIR | 0755;
    st->st_nlink = 0;
  } else {
    st->st_mode = S_IFREG | 0644;
    st->st_nlink = 0;
    st->st_size =
        m_file_info->data_loc_size + m_file_info->shared_data.shared_data_size;
  }
  return 0;
}

int lstat(const char *path, s_stat *st) {
  st->st_uid = 0;
  st->st_gid = 0;
  st->st_atime = 0;
  st->st_ctime = 0;

  char sh_buf[SECTOR_SIZE];
  if (!ssfs_read_dev(0, 1, sh_buf))
    return -1;

  ssfs_header *sh = malloc(sizeof(ssfs_header));
  memcpy(sh, sh_buf, sizeof(ssfs_header));

  uint32_t dirent_table_size = sh->total_dirents + 1;

  // Read the SSFS dirent table
  ssfs_dirent dirent_table[dirent_table_size];
  if (ssfs_read_dev(1,
                    ((sizeof(ssfs_dirent) * sh->total_dirents) + SECTOR_SIZE) /
                        SECTOR_SIZE,
                    dirent_table))
    return -1;

  free(sh);

  ssfs_dirent file_dirent =
      ssfs_get_dirent_from_info(ssfs_get_filename_info((char*)path), dirent_table);
          
  ssfs_dirent dirent;
  if (file_dirent.is_link)
    dirent = dirent_table[file_dirent.link_dirent_table_index];
  else dirent = file_dirent;

  uint64_t i = 0;
  char
      m_dirent_inode_buf[((dirent.inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE) *
                         SECTOR_SIZE];
  if (ssfs_read_dev(dirent.inode_loc.start_lba,
                    (dirent.inode_loc.size + SECTOR_SIZE) / SECTOR_SIZE,
                    m_dirent_inode_buf))
    return -1;

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

  if (strcmp(path, "/") == 0) {
    st->st_mode = S_IFDIR | 0755;
    st->st_nlink = 0;
  } else {
    st->st_mode = S_IFREG | 0644;
    st->st_nlink = 0;
    st->st_size =
        m_file_info->data_loc_size + m_file_info->shared_data.shared_data_size;
  }
  return 0;
}