#include <ctype.h>
#include "tsk_fs_i.h"
#include "tsk_xfs.h"

TSK_RETVAL_ENUM
xfs_dir_open_meta(TSK_FS_INFO * a_fs, TSK_FS_DIR ** a_fs_dir,
    TSK_INUM_T a_addr)
{
    XFS_INFO *xfs = (XFS_INFO *) a_fs;
    char *dirbuf;
    TSK_OFF_T size;
    TSK_FS_DIR *fs_dir;
    TSK_LIST *list_seen = NULL;

    /* If we get corruption in one of the blocks, then continue processing.
     * retval_final will change when corruption is detected.  Errors are
     * returned immediately. */
    TSK_RETVAL_ENUM retval_tmp;
    TSK_RETVAL_ENUM retval_final = TSK_OK;

    if (a_addr < a_fs->first_inum || a_addr > a_fs->last_inum) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_WALK_RNG);
        tsk_error_set_errstr("xfs_dir_open_meta: inode value: %"
            PRIuINUM "\n", a_addr);
        return TSK_ERR;
    }
    else if (a_fs_dir == NULL) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr
            ("xfs_dir_open_meta: NULL fs_attr argument given");
        return TSK_ERR;
    }

    if (tsk_verbose) {
        tsk_fprintf(stderr,
            "xfs_dir_open_meta: Processing directory %" PRIuINUM
            "\n", a_addr);
    }

    fs_dir = *a_fs_dir;
    if (fs_dir) {
        tsk_fs_dir_reset(fs_dir);
        fs_dir->addr = a_addr;
    }
    else {
        if ((*a_fs_dir = fs_dir =
                tsk_fs_dir_alloc(a_fs, a_addr, 128)) == NULL) {
            return TSK_ERR;
        }
    }

    //  handle the orphan directory if its contents were requested
    if (a_addr == TSK_FS_ORPHANDIR_INUM(a_fs)) {
// #ifdef Ext4_DBG
//         tsk_fprintf(stderr, "DEBUG: Getting ready to process ORPHANS\n");
// #endif
        return tsk_fs_dir_find_orphans(a_fs, fs_dir);
    }
//     else {
// #ifdef Ext4_DBG
//         tsk_fprintf(stderr,
//             "DEBUG: not orphan %" PRIuINUM "!=%" PRIuINUM "\n", a_addr,
//             TSK_FS_ORPHANDIR_INUM(a_fs));
// #endif
//     }

    // if ((fs_dir->fs_file =
    //         tsk_fs_file_open_meta(a_fs, NULL, a_addr)) == NULL) {
    //     tsk_error_reset();
    //     tsk_error_errstr2_concat("- xfs_dir_open_meta");
    //     return TSK_COR;
    // }

    // // We only read in and process a single block at a time
    // if ((dirbuf = tsk_malloc((size_t)a_fs->block_size)) == NULL) {
    //     return TSK_ERR;
    // }

    // size = roundup(fs_dir->fs_file->meta->size, a_fs->block_size);
    // TSK_OFF_T offset = 0;

    // while (size > 0) {
    //     ssize_t len = (a_fs->block_size < size) ? a_fs->block_size : size;
    //     ssize_t cnt = tsk_fs_file_read(fs_dir->fs_file, offset, dirbuf, len, (TSK_FS_FILE_READ_FLAG_ENUM)0);
    //     if (cnt != len) {
    //         tsk_error_reset();
    //         tsk_error_set_errno(TSK_ERR_FS_FWALK);
    //         tsk_error_set_errstr
    //         ("ext2fs_dir_open_meta: Error reading directory contents: %"
    //             PRIuINUM "\n", a_addr);
    //         free(dirbuf);
    //         return TSK_COR;
    //     }

    //     retval_tmp =
    //         ext2fs_dent_parse_block(ext2fs, fs_dir,
    //         (fs_dir->fs_file->meta->
    //             flags & TSK_FS_META_FLAG_UNALLOC) ? 1 : 0, &list_seen,
    //         dirbuf, len);

    //     if (retval_tmp == TSK_ERR) {
    //         retval_final = TSK_ERR;
    //         break;
    //     }
    //     else if (retval_tmp == TSK_COR) {
    //         retval_final = TSK_COR;
    //     }

    //     size -= len;
    //     offset += len;
    // }
    // free(dirbuf);

    // // if we are listing the root directory, add the Orphan directory entry
    // if (a_addr == a_fs->root_inum) {
    //     TSK_FS_NAME *fs_name = tsk_fs_name_alloc(256, 0);
    //     if (fs_name == NULL)
    //         return TSK_ERR;

    //     if (tsk_fs_dir_make_orphan_dir_name(a_fs, fs_name)) {
    //         tsk_fs_name_free(fs_name);
    //         return TSK_ERR;
    //     }

    //     if (tsk_fs_dir_add(fs_dir, fs_name)) {
    //         tsk_fs_name_free(fs_name);
    //         return TSK_ERR;
    //     }
    //     tsk_fs_name_free(fs_name);
    // }

    return retval_final;
}
