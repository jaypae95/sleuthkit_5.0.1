#include "tsk_fs_i.h"
#include "tsk_xfs.h"

TSK_FS_INFO *
xfs_open(TSK_IMG_INFO * img_info, TSK_OFF_T offset,
    TSK_FS_TYPE_ENUM ftype, uint8_t test) {
        
    printf("hello tsk I'm mingi\n");

    XFS_INFO *xfs;
    unsigned int len;
    TSK_FS_INFO *fs;
    ssize_t cnt;

    // clean up any error messages that are lying around
    tsk_error_reset();

    if (TSK_FS_TYPE_ISXFS(ftype) == 0) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr("Invalid FS Type in xfs_open");
        return NULL;
    }

    if (img_info->sector_size == 0) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr("xfs_open: sector size is 0");
        return NULL;
    }

    if ((xfs = (XFS_INFO *) tsk_fs_malloc(sizeof(*xfs))) == NULL)
        return NULL;

    fs = &(xfs->fs_info);

    fs->ftype = ftype;
    fs->flags = 0;
    fs->img_info = img_info;
    fs->offset = offset;
    fs->tag = TSK_FS_INFO_TAG;

    /*
     * Read the superblock.
     */
    len = sizeof(xfs);
    if ((xfs->fs = (xfs_sb *) tsk_malloc(len)) == NULL) {
        fs->tag = 0;
        tsk_fs_free((TSK_FS_INFO *)xfs);
        return NULL;
    }

    cnt = tsk_fs_read(fs, XFS_SBOFF, (char *) xfs->fs, len);
    if (cnt != len) {
        if (cnt >= 0) {
            tsk_error_reset();
            tsk_error_set_errno(TSK_ERR_FS_READ);
        }
        tsk_error_set_errstr2("xfs_open: superblock");
        fs->tag = 0;
        free(xfs->fs);
        tsk_fs_free((TSK_FS_INFO *)xfs);
        return NULL;
    }

    /*
     * Verify we are looking at an XFS image
     */
    // magic num을 확인하고 big endian인지 little endian인지 정한다.
    if (tsk_fs_guessu32(fs, xfs->fs->sb_magicnum, XFS_FS_MAGIC)) {
        fs->tag = 0;
        free(xfs->fs);
        tsk_fs_free((TSK_FS_INFO *)xfs);
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_MAGIC);
        tsk_error_set_errstr("not an XFS file system (magic)");
        if (tsk_verbose)
            fprintf(stderr, "xfs_open: invalid magic\n");
        return NULL;
    }

    
    return (fs);
}