#include <ctype.h>
#include <string.h>
#include "tsk_fs_i.h"
#include "tsk_xfs.h"

ssize_t
xfs_fs_file_read(TSK_FS_FILE * a_fs_file,
    TSK_OFF_T a_offset, char *a_buf, size_t a_len,
    TSK_FS_FILE_READ_FLAG_ENUM a_flags){

    ssize_t cnt;
    XFS_INFO* xfs = (XFS_INFO *) a_fs_file->fs_info;
    TSK_FS_INFO *fs = a_fs_file->fs_info;
    int format = a_fs_file->meta->inode_format;
    TSK_DADDR_T start_addr;
    TSK_DADDR_T *addr_ptr;
    addr_ptr = (TSK_DADDR_T *) a_fs_file->meta->content_ptr;
    start_addr = addr_ptr[0];

    size_t size = 0;
    if (format == 0x1) { // shortformat
        size = 0x150;
        cnt = tsk_fs_read(fs, start_addr, (char *) a_buf, size);
    } else if (format == 0x2) {// extent
        size = tsk_getu32(fs->endian, xfs->fs->sb_blocksize) * (2 << xfs->fs->sb_dirblklog);
        cnt = tsk_fs_read(fs, start_addr, (char *) a_buf, size);
    }

    return cnt;
}

static uint8_t
xfs_dent_copy(XFS_INFO * xfs,
    char *xfs_dent, TSK_FS_NAME * fs_name)
{
    TSK_FS_INFO *fs = &(xfs->fs_info);
    xfs_dir2_sf_entry_t *dir = (xfs_dir2_sf_entry_t *) xfs_dent;

        // todo case1 i4 and i8
        fs_name->meta_addr = tsk_getu32(fs->endian, dir->inumber.i4.i);

        /* xfs does not null terminate */
        if (dir->namelen >= fs_name->name_size) {
            tsk_error_reset();
            tsk_error_set_errno(TSK_ERR_FS_ARG);
            tsk_error_set_errstr
                ("xfs_dent_copy: Name Space too Small %d %" PRIuSIZE "",
                dir->namelen, fs_name->name_size);
            return 1;
        }

        /* Copy and Null Terminate */
        strncpy(fs_name->name, dir->name, dir->namelen);
        fs_name->name[dir->namelen] = '\0';

        switch (dir->ftype) {
        case XFS_DE_REG:
            fs_name->type = TSK_FS_NAME_TYPE_REG;
            break;
        case XFS_DE_DIR:
            fs_name->type = TSK_FS_NAME_TYPE_DIR;
            break;
        case XFS_DE_CHR:
            fs_name->type = TSK_FS_NAME_TYPE_CHR;
            break;
        case XFS_DE_BLK:
            fs_name->type = TSK_FS_NAME_TYPE_BLK;
            break;
        case XFS_DE_FIFO:
            fs_name->type = TSK_FS_NAME_TYPE_FIFO;
            break;
        case XFS_DE_SOCK:
            fs_name->type = TSK_FS_NAME_TYPE_SOCK;
            break;
        case XFS_DE_LNK:
            fs_name->type = TSK_FS_NAME_TYPE_LNK;
            break;
        case XFS_DE_UNKNOWN:
        default:
            fs_name->type = TSK_FS_NAME_TYPE_UNDEF;
            break;
        }
}
/*
 * @param a_is_del Set to 1 if block is from a deleted directory.
 */
// static TSK_RETVAL_ENUM
// xfs_dent_parse_block(XFS_INFO * xfs, TSK_FS_DIR * a_fs_dir,
//     uint8_t a_is_del, TSK_LIST ** list_seen, char *buf, int len)
static TSK_RETVAL_ENUM
xfs_dent_parse_block(XFS_INFO * xfs, TSK_FS_DIR * a_fs_dir,
    uint8_t a_is_del, TSK_LIST ** list_seen, char *buf, int len)
{
    TSK_FS_INFO *fs = &(xfs->fs_info);

    int dellen = 0;
    int idx = 0;
    uint16_t reclen;
    uint32_t inode;
    char *dirPtr;
    TSK_FS_NAME *fs_name;
    int minreclen = 4;
    ssize_t cnt;

    if ((fs_name = tsk_fs_name_alloc(XFS_MAXNAMLEN + 1, 0)) == NULL)
        return TSK_ERR;

    /* update each time by the actual length instead of the
     ** recorded length so we can view the deleted entries

        we don't consider deleted entries
     */
    size_t headerlen = 0;
    TSK_FS_FILE* fs_file = a_fs_dir->fs_file;
    xfs_dir2_sf_hdr_t *hdr;
    if (fs_file->meta->inode_format == 0x1) {
        dirPtr = &buf[idx];
        hdr = (xfs_dir2_sf_hdr_t*)dirPtr;
        idx += sizeof(xfs_dir2_sf_hdr_t) - 4;
    }

    xfs_dir2_sf_entry_t *sf_entry;
    char name[XFS_MAXNAMLEN];
    for (int i = 0; i < hdr->count; i++) {
        dirPtr = &buf[idx];
        
        sf_entry = (xfs_dir2_sf_entry_t *)dirPtr;
        uint16_t offset = tsk_getu16(fs->endian, sf_entry->offset);

        idx += sizeof(uint8_t) * 3 + sf_entry->namelen;
        sf_entry->ftype = (uint8_t) buf[idx];
        idx += sizeof(uint8_t);
        xfs_dir2_inou_t * inumber = (xfs_dir2_inou_t *)&buf[idx];
        sf_entry->inumber = (*inumber);
        idx += sizeof(xfs_dir2_inou_t) - 4;

        if (hdr->i8count == 0) {
            fs_name->meta_addr = tsk_getu32(fs->endian, sf_entry->inumber.i4.i);
        } else {
            fs_name->meta_addr = tsk_getu64(fs->endian, sf_entry->inumber.i8.i);
        }
        strncpy(fs_name->name, sf_entry->name, sf_entry->namelen);
        fs_name->name[sf_entry->namelen] = '\0';
        switch (sf_entry->ftype) {
        case XFS_DE_REG:
            fs_name->type = TSK_FS_NAME_TYPE_REG;
            break;
        case XFS_DE_DIR:
            fs_name->type = TSK_FS_NAME_TYPE_DIR;
            break;
        case XFS_DE_CHR:
            fs_name->type = TSK_FS_NAME_TYPE_CHR;
            break;
        case XFS_DE_BLK:
            fs_name->type = TSK_FS_NAME_TYPE_BLK;
            break;
        case XFS_DE_FIFO:
            fs_name->type = TSK_FS_NAME_TYPE_FIFO;
            break;
        case XFS_DE_SOCK:
            fs_name->type = TSK_FS_NAME_TYPE_SOCK;
            break;
        case XFS_DE_LNK:
            fs_name->type = TSK_FS_NAME_TYPE_LNK;
            break;
        case XFS_DE_UNKNOWN:
        default:
            fs_name->type = TSK_FS_NAME_TYPE_UNDEF;
            break;
        }
        fs_name->flags = TSK_FS_NAME_FLAG_ALLOC;
        if (tsk_fs_dir_add(a_fs_dir, fs_name)) {
            tsk_fs_name_free(fs_name);
            return TSK_ERR;
        }
    }

    
    tsk_fs_name_free(fs_name);
    return TSK_OK;
}

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
    else if ((*a_fs_dir = fs_dir =
                tsk_fs_dir_alloc(a_fs, a_addr, 128)) == NULL) {
            return TSK_ERR;
    }
    

    if ((fs_dir->fs_file =
            tsk_fs_file_open_meta(a_fs, NULL, a_addr)) == NULL) {
        tsk_error_reset();
        tsk_error_errstr2_concat("- xfs_dir_open_meta");
        return TSK_ERR;
    }

    // // We only read in and process a single block at a time
    if ((dirbuf = (char *)tsk_malloc((size_t)a_fs->block_size)) == NULL) {
        return TSK_ERR;
    }

    size = roundup(fs_dir->fs_file->meta->size, a_fs->block_size);
    TSK_OFF_T offset = 0;

    while (size > 0) {
        ssize_t len = (a_fs->block_size < size) ? a_fs->block_size : size;
        ssize_t cnt = xfs_fs_file_read(fs_dir->fs_file, offset, dirbuf, len, (TSK_FS_FILE_READ_FLAG_ENUM)0);
        retval_tmp =
            xfs_dent_parse_block(xfs, fs_dir,
            (fs_dir->fs_file->meta->
                flags & TSK_FS_META_FLAG_UNALLOC) ? 1 : 0, &list_seen,
            dirbuf, len);

        if (retval_tmp == TSK_ERR) {
            retval_final = TSK_ERR;
            break;
        }
        else if (retval_tmp == TSK_COR) {
            retval_final = TSK_COR;
        }

        size -= len;
        offset += len;
    }
    free(dirbuf);

    // if we are listing the root directory, add the Orphan directory entry
    // if (a_addr == a_fs->root_inum) {
    //     TSK_FS_NAME *fs_name = tsk_fs_name_alloc(256, 0);
    //     if (fs_name == NULL)
    //         return TSK_ERR;

    //     if (tsk_fs_dir_add(fs_dir, fs_name)) {
    //         tsk_fs_name_free(fs_name);
    //         return TSK_ERR;
    //     }
    //     tsk_fs_name_free(fs_name);
    // }

    return retval_final;
}
