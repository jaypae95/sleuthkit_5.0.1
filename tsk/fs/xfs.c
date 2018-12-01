#include "tsk_fs_i.h"
#include "tsk_xfs.h"
#include <sys/stat.h>

#define xfs_cgbase_lcl(fsi, fs, c)	\
	tsk_getu32(fsi->endian, fs->sb_agblocks)*c

/** \internal
 * Add the data runs and extents to the file attributes.
 *
 * @param fs_file File system to analyze
 * @returns 0 on success, 1 otherwise
 */
static uint8_t
ext2fs_load_attrs(TSK_FS_FILE * fs_file)
{
    /* EXT4 extents-based storage is dealt with differently than
     * the traditional pointer lists. */
    if (fs_file->meta->content_type == TSK_FS_META_CONTENT_TYPE_EXT4_EXTENTS) {
        return ext4_load_attrs_extents(fs_file);
    }
    else {
        return tsk_fs_unix_make_data_run(fs_file);
    }
}


/* xfs_dinode_copy - copy cached disk inode into generic inode
 *
 * returns 1 on error and 0 on success
 * */
static uint8_t
xfs_dinode_copy(XFS_INFO * xfs, TSK_FS_META * fs_meta,
    TSK_INUM_T inum, const xfs_dinode * dino_buf)
{
    int i;
    TSK_FS_INFO *fs = (TSK_FS_INFO *) & xfs->fs_info;
    xfs_sb *sb = xfs->fs;
    TSK_INUM_T ibase = 0;


    if (dino_buf == NULL) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr("ext2fs_dinode_copy: dino_buf is NULL");
        return 1;
    }

    fs_meta->attr_state = TSK_FS_META_ATTR_EMPTY;
    if (fs_meta->attr) {
        tsk_fs_attrlist_markunused(fs_meta->attr);
    }

    // set the type
    switch (tsk_getu16(fs->endian, dino_buf->di_core.di_mode) & XFS_IN_FMT) {
    case __S_IFREG:
        fs_meta->type = TSK_FS_META_TYPE_REG;
        break;
    case __S_IFDIR:
        fs_meta->type = TSK_FS_META_TYPE_DIR;
        break;
    case __S_IFSOCK:
        fs_meta->type = TSK_FS_META_TYPE_SOCK;
        break;
    case __S_IFLNK:
        fs_meta->type = TSK_FS_META_TYPE_LNK;
        break;
    case __S_IFBLK:
        fs_meta->type = TSK_FS_META_TYPE_BLK;
        break;
    case __S_IFCHR:
        fs_meta->type = TSK_FS_META_TYPE_CHR;
        break;
    case __S_IFIFO:
        fs_meta->type = TSK_FS_META_TYPE_FIFO;
        break;
    default:
        fs_meta->type = TSK_FS_META_TYPE_UNDEF;
        break;
    }

    // set the mode
    fs_meta->mode = 0;
    if (tsk_getu16(fs->endian, dino_buf->di_core.di_mode) & S_ISUID)
        fs_meta->mode |= TSK_FS_META_MODE_ISUID;
    if (tsk_getu16(fs->endian, dino_buf->di_core.di_mode) & S_ISGID)
        fs_meta->mode |= TSK_FS_META_MODE_ISGID;
    if (tsk_getu16(fs->endian, dino_buf->di_core.di_mode) & __S_ISVTX)
        fs_meta->mode |= TSK_FS_META_MODE_ISVTX;

    if (tsk_getu16(fs->endian, dino_buf->di_core.di_mode) & S_IRUSR)
        fs_meta->mode |= TSK_FS_META_MODE_IRUSR;
    if (tsk_getu16(fs->endian, dino_buf->di_core.di_mode) & S_IWUSR)
        fs_meta->mode |= TSK_FS_META_MODE_IWUSR;
    if (tsk_getu16(fs->endian, dino_buf->di_core.di_mode) & S_IXUSR)
        fs_meta->mode |= TSK_FS_META_MODE_IXUSR;

    if (tsk_getu16(fs->endian, dino_buf->di_core.di_mode) & S_IRGRP)
        fs_meta->mode |= TSK_FS_META_MODE_IRGRP;
    if (tsk_getu16(fs->endian, dino_buf->di_core.di_mode) & S_IWGRP)
        fs_meta->mode |= TSK_FS_META_MODE_IWGRP;
    if (tsk_getu16(fs->endian, dino_buf->di_core.di_mode) & S_IXGRP)
        fs_meta->mode |= TSK_FS_META_MODE_IXGRP;

    if (tsk_getu16(fs->endian, dino_buf->di_core.di_mode) & S_IROTH)
        fs_meta->mode |= TSK_FS_META_MODE_IROTH;
    if (tsk_getu16(fs->endian, dino_buf->di_core.di_mode) & S_IWOTH)
        fs_meta->mode |= TSK_FS_META_MODE_IWOTH;
    if (tsk_getu16(fs->endian, dino_buf->di_core.di_mode) & S_IXOTH)
        fs_meta->mode |= TSK_FS_META_MODE_IXOTH;

    fs_meta->nlink = tsk_getu16(fs->endian, dino_buf->di_core.di_nlink);

    fs_meta->size = tsk_getu64(fs->endian, dino_buf->di_core.di_size);

    fs_meta->addr = inum;

    // /* the general size value in the inode is only 32-bits,
    //  * but the i_dir_acl value is used for regular files to
    //  * hold the upper 32-bits
    //  *
    //  * The RO_COMPAT_LARGE_FILE flag in the super block will identify
    //  * if there are any large files in the file system
    //  */
    // if ((fs_meta->type == TSK_FS_META_TYPE_REG) &&
    //     (tsk_getu32(fs->endian, sb->s_feature_ro_compat) &
    //         EXT2FS_FEATURE_RO_COMPAT_LARGE_FILE)) {
    //     fs_meta->size +=
    //         ((uint64_t) tsk_getu32(fs->endian,
    //             dino_buf->i_size_high) << 32);
    // }

    fs_meta->uid =
        tsk_getu32(fs->endian, dino_buf->di_core.di_uid);

    fs_meta->gid =
        tsk_getu32(fs->endian, dino_buf->di_core.di_gid);

    fs_meta->mtime = tsk_getu32(fs->endian, dino_buf->di_core.di_mtime);
    fs_meta->atime = tsk_getu32(fs->endian, dino_buf->di_core.di_atime);
    fs_meta->ctime = tsk_getu32(fs->endian, dino_buf->di_core.di_ctime);
    // fs_meta->time2.ext2.dtime = tsk_getu32(fs->endian, dino_buf->di_core.di_dtime);
    // if (fs->ftype == TSK_FS_TYPE_EXT4) {
    //     fs_meta->mtime_nano =
    //         tsk_getu32(fs->endian, dino_buf->i_mtime_extra) >> 2;
    //     fs_meta->atime_nano =
    //         tsk_getu32(fs->endian, dino_buf->i_atime_extra) >> 2;
    //     fs_meta->ctime_nano =
    //         tsk_getu32(fs->endian, dino_buf->i_ctime_extra) >> 2;
    //     fs_meta->crtime = tsk_getu32(fs->endian, dino_buf->i_crtime);
    //     fs_meta->crtime_nano =
    //         tsk_getu32(fs->endian, dino_buf->i_crtime_extra) >> 2;
    // }
    // else {
    //     fs_meta->mtime_nano = fs_meta->atime_nano = fs_meta->ctime_nano = 0;
    //     fs_meta->crtime = 0;
    // }
    // fs_meta->time2.ext2.dtime_nano = 0;
    fs_meta->seq = 0;

    if (fs_meta->link) {
        free(fs_meta->link);
        fs_meta->link = NULL;
    }

    if (fs_meta->content_len != XFS_FILE_CONTENT_LEN) {
        if ((fs_meta =
                tsk_fs_meta_realloc(fs_meta,
                    XFS_FILE_CONTENT_LEN)) == NULL) {
            return 1;
        }
    }

    TSK_OFF_T addr;
    ssize_t cnt;
    xfs_dinode_core *dino_core_buf = &(dino_buf->di_core);

    uint16_t inodesize = tsk_getu16(fs->endian, xfs->fs->sb_inodesize);

    
    xfs_dir2_sf_t * di_dir2sf = &(dino_buf->di_u.di_dir2sf);
    // todo b tree ~~  
    if (dino_core_buf->di_format == 0x1) { // short form
        fs_meta->inode_format = 0x1;
        addr = tsk_getu64(fs->endian, dino_core_buf->di_ino) * inodesize + sizeof(xfs_dinode_core);
        TSK_DADDR_T *addr_ptr;
        // fs_meta->content_type = TSK_FS_META_CONTENT_TYPE_EXT4_EXTENTS;
        /* NOTE TSK_DADDR_T != uint32_t, so lets make sure we use uint32_t */
        addr_ptr = (TSK_DADDR_T *) fs_meta->content_ptr;
        addr_ptr[0] = addr;
    }
    else if (dino_core_buf->di_format == 0x2) {  // extent
        // extent list
        fs_meta->inode_format = 0x2;
        xfs_dir2_sf_t *di_bmx;
        for (int i = 0; i < tsk_getu32(fs->endian, dino_core_buf->di_nextents); i++) {
            di_bmx = &(dino_buf->di_u.di_bmx[i]);
            cnt = tsk_fs_read(fs, addr, (char *) di_bmx, sizeof(xfs_bmbt_rec_32_t));
            if (cnt != sizeof(xfs_bmbt_rec_32_t)) {
                if (cnt >= 0) {
                    tsk_error_reset();
                    tsk_error_set_errno(TSK_ERR_FS_READ);
                }
                tsk_error_set_errstr2("xfs_dinode_load: Inode %" PRIuINUM
                    " from %" PRIuOFF, inum, addr);
                return 1;
            }


        }
    }

    

    /*
     * Apply the used/unused restriction.
     */
    fs_meta->flags |= (fs_meta->ctime ?
        TSK_FS_META_FLAG_USED : TSK_FS_META_FLAG_UNUSED);

    return 0;
}

/* xfs_dinode_load - look up disk inode & load into xfs_inode structure
 * @param xfs A xfs file system information structure
 * @param dino_inum Metadata address
 * @param dino_buf The buffer to store the block in (must be size of xfs->inode_size or larger)
 *
 * return 1 on error and 0 on success
 * */

static uint8_t
xfs_dinode_load(XFS_INFO * xfs, TSK_INUM_T dino_inum,
    xfs_dinode * dino_buf)
{
    xfs_dinode_core *dino_core_buf = &(dino_buf->di_core);
    TSK_FS_INFO *fs = (TSK_FS_INFO *) & xfs->fs_info;
    TSK_OFF_T addr;
    ssize_t cnt;

    if ((dino_inum < fs->first_inum) || (dino_inum > fs->last_inum - 1))
    {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_INODE_NUM);
        tsk_error_set_errstr("ext2fs_dinode_load: address: %" PRIuINUM, dino_inum);
        return 1;
    }
    if (dino_buf == NULL)
    {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr("ext2fs_dinode_load: dino_buf is NULL");
        return 1;
    }                                                                    /*      * Look up the group descriptor for this inode.      */
    tsk_take_lock(&xfs->lock);

    uint64_t root_ino = tsk_getu64(fs->endian, xfs->fs->sb_rootino);
    uint16_t inodesize = tsk_getu16(fs->endian, xfs->fs->sb_inodesize);
    //addr = tsk_getu64(fs->endian, xfs->fs->sb_rootino) * tsk_getu16(fs->endian, xfs->fs->sb_inodesize);
    addr = root_ino * inodesize;
    
    cnt = tsk_fs_read(fs, addr, (char *)dino_core_buf, sizeof(xfs_dinode_core));
    if (cnt != sizeof(xfs_dinode_core)) {
        if (cnt >= 0) {
            tsk_error_reset();
            tsk_error_set_errno(TSK_ERR_FS_READ);
        }
        tsk_error_set_errstr2("xfs_dinode_load: Inode %" PRIuINUM
            " from %" PRIuOFF, dino_inum, addr);
        return 1;
    }
    
    switch (dino_core_buf->di_format) {
        case 0x0:
            //
            break;
        case 0x1:
            // local inode 
            
            break;
        case 0x2:
            // extent list
            addr = addr + sizeof(xfs_dinode_core);
            xfs_dir2_sf_t *di_bmx;
            for (int i = 0; i < tsk_getu32(fs->endian, dino_core_buf->di_nextents); i++) {
                di_bmx = &(dino_buf->di_u.di_bmx[i]);
                cnt = tsk_fs_read(fs, addr, (char *) di_bmx, sizeof(xfs_bmbt_rec_32_t));
                if (cnt != sizeof(xfs_bmbt_rec_32_t)) {
                    if (cnt >= 0) {
                        tsk_error_reset();
                        tsk_error_set_errno(TSK_ERR_FS_READ);
                    }
                    tsk_error_set_errstr2("xfs_dinode_load: Inode %" PRIuINUM
                        " from %" PRIuOFF, dino_inum, addr);
                    return 1;
                }
            }

            break;
        case 0x3:
                // btree root follows
            break;
    }

    tsk_release_lock(&xfs->lock);

    return 0;
}
/* xfs_inode_walk - inode iterator
 *
 * flags used: TSK_FS_META_FLAG_USED, TSK_FS_META_FLAG_UNUSED,
 *  TSK_FS_META_FLAG_ALLOC, TSK_FS_META_FLAG_UNALLOC, TSK_FS_META_FLAG_ORPHAN
 *
 *  Return 1 on error and 0 on success
*/

uint8_t
xfs_inode_walk(TSK_FS_INFO * fs, TSK_INUM_T start_inum,
    TSK_INUM_T end_inum, TSK_FS_META_FLAG_ENUM flags,
    TSK_FS_META_WALK_CB a_action, void *a_ptr)
{
    char *myname = "xfs_inode_walk";
    XFS_INFO *xfs = (XFS_INFO *) fs;
    TSK_INUM_T inum;
    TSK_INUM_T end_inum_tmp;
    TSK_INUM_T ibase = 0;
    TSK_FS_FILE *fs_file;
    unsigned int myflags;
    xfs_dinode_core *dino_buf = NULL;
    unsigned int size = 0;

    // clean up any error messages that are lying around
    tsk_error_reset();




    /*
     * Cleanup.
     */
    tsk_fs_file_close(fs_file);
    free(dino_buf);

    return 0;
}

/* ext2fs_block_walk - block iterator
 *
 * flags: TSK_FS_BLOCK_FLAG_ALLOC, TSK_FS_BLOCK_FLAG_UNALLOC, TSK_FS_BLOCK_FLAG_CONT,
 *  TSK_FS_BLOCK_FLAG_META
 *
 *  Return 1 on error and 0 on success
*/

uint8_t
xfs_block_walk(TSK_FS_INFO * a_fs, TSK_DADDR_T a_start_blk,
    TSK_DADDR_T a_end_blk, TSK_FS_BLOCK_WALK_FLAG_ENUM a_flags,
    TSK_FS_BLOCK_WALK_CB a_action, void *a_ptr)
{
    char *myname = "xfs_block_walk";
    TSK_FS_BLOCK *fs_block;
    TSK_DADDR_T addr;

    // clean up any error messages that are lying around
    tsk_error_reset();





    /*
     * Cleanup.
     */
    tsk_fs_block_free(fs_block);
    return 0;
}

/* xfs_inode_lookup - lookup inode, external interface
 *
 * @param fs File system to read from.
 * @param a_fs_file Structure to read into.
 * @param inum File address to load
 * @returns 1 on error
 */

static uint8_t
xfs_inode_lookup(TSK_FS_INFO * fs, TSK_FS_FILE * a_fs_file,
    TSK_INUM_T inum)
{
    XFS_INFO *xfs = (XFS_INFO *) fs;
    xfs_dinode *dino_buf = NULL;
    unsigned int size = 0;

    if (a_fs_file == NULL) {
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr("xfs_inode_lookup: fs_file is NULL");
        return 1;
    }

    if (a_fs_file->meta == NULL) {
        a_fs_file->meta = tsk_fs_meta_alloc(XFS_FILE_CONTENT_LEN);
    }

    if (a_fs_file->meta == NULL) {
        return 1;
    }
    else {
        tsk_fs_meta_reset(a_fs_file->meta);
    }

    // see if they are looking for the special "orphans" directory
    if (inum == TSK_FS_ORPHANDIR_INUM(fs)) {
        if (tsk_fs_dir_make_orphan_dir_meta(fs, a_fs_file->meta))
            return 1;
        else
            return 0;
    }

    size =
        xfs->inode_size >
        sizeof(xfs_dinode) ? xfs->inode_size : sizeof(xfs_dinode);
    if ((dino_buf = (xfs_dinode *) tsk_malloc(size)) == NULL) {
        return 1;
    }

    if (xfs_dinode_load(xfs, inum, dino_buf)) {
        free(dino_buf);
        return 1;
    }

    if (xfs_dinode_copy(xfs, a_fs_file->meta, inum, dino_buf)) {
        free(dino_buf);
        return 1;
    }

    free(dino_buf);
    return 0;
}


static void
xfs_close(TSK_FS_INFO * fs)
{
    XFS_INFO *xfs = (XFS_INFO *) fs;

    fs->tag = 0;
    free(xfs->fs);
    free(xfs->bmap_buf);
    free(xfs->imap_buf);

    tsk_deinit_lock(&xfs->lock);

    tsk_fs_free(fs);
}

/**
 * Print details about the file system to a file handle.
 *
 * @param fs File system to print details on
 * @param hFile File handle to print text to
 *
 * @returns 1 on error and 0 on success
 */
static uint8_t
xfs_fsstat(TSK_FS_INFO * fs, FILE * hFile)
{
    unsigned int i;
//    unsigned int gpfbg;
//    unsigned int gd_blocks;
    XFS_INFO *xfs = (XFS_INFO *) fs;
    xfs_sb *sb = xfs->fs;
    int ibpg;
    time_t tmptime;
    char timeBuf[128];
    uint64_t inode_per_ag = tsk_getu64(fs->endian, sb->sb_icount)/4;

    // clean up any error messages that are lying around
    tsk_error_reset();

    tsk_fprintf(hFile, "FILE SYSTEM INFORMATION\n");
    tsk_fprintf(hFile, "--------------------------------------------\n");

    tsk_fprintf(hFile, "File System Type: %s\n", "XFS");
    tsk_fprintf(hFile, "Volume Name: %s\n", sb->sb_fname);
    tsk_fprintf(hFile, "Volume ID: %" PRIx64 "%" PRIx64 "\n",
        tsk_getu64(fs->endian, &sb->sb_uuid[8]), tsk_getu64(fs->endian,
            &sb->sb_uuid[0]));
    
    tsk_fprintf(hFile, "Features Compat: %" PRIu32 "\n",
        tsk_getu32(fs->endian, sb->sb_features_cmpt));
    
    tsk_fprintf(hFile, "Features Read-Only Compat: %" PRIu32 "\n",
        tsk_getu32(fs->endian, sb->sb_features_rocmpt));

    tsk_fprintf(hFile, "Features Incompat: %" PRIu32 "\n",
        tsk_getu32(fs->endian, sb->sb_features_incmpt));

    tsk_fprintf(hFile, "CRC: %" PRIu32 "\n",
        tsk_getu32(fs->endian, sb->sb_checksum));

    // /* Print journal information */
    // if (tsk_getu32(fs->endian, sb->s_feature_compat) &
    //     EXT2FS_FEATURE_COMPAT_HAS_JOURNAL) {

    //     tsk_fprintf(hFile, "\nJournal ID: %" PRIx64 "%" PRIx64 "\n",
    //         tsk_getu64(fs->endian, &sb->s_journal_uuid[8]),
    //         tsk_getu64(fs->endian, &sb->s_journal_uuid[0]));

    //     if (tsk_getu32(fs->endian, sb->s_journal_inum) != 0)
    //         tsk_fprintf(hFile, "Journal Inode: %" PRIu32 "\n",
    //             tsk_getu32(fs->endian, sb->s_journal_inum));

    //     if (tsk_getu32(fs->endian, sb->s_journal_dev) != 0)
    //         tsk_fprintf(hFile, "Journal Device: %" PRIu32 "\n",
    //             tsk_getu32(fs->endian, sb->s_journal_dev));


    // }

    tsk_fprintf(hFile, "\nMETADATA INFORMATION\n");
    tsk_fprintf(hFile, "--------------------------------------------\n");

    tsk_fprintf(hFile, "Inode Range: %" PRIuINUM " - %" PRIuINUM "\n",
        fs->first_inum, fs->last_inum);
    tsk_fprintf(hFile, "Root Directory: %" PRIuINUM "\n", fs->root_inum);

    tsk_fprintf(hFile, "Free Inodes: %" PRIuINUM "\n",
        tsk_getu64(fs->endian, sb->sb_ifree));

    tsk_fprintf(hFile, "Inode Size: %" PRIu16 "\n",
        tsk_getu16(fs->endian, sb->sb_inodesize));

    tsk_fprintf(hFile, "Extent Size: %" PRIu16 "\n",
        tsk_getu16(fs->endian, sb->sb_sectsize));

    tsk_fprintf(hFile, "Free Extent Count: %" PRIu64 "\n",
        tsk_getu64(fs->endian, sb->sb_frextents));

    
    tsk_fprintf(hFile, "\nCONTENT INFORMATION\n");
    tsk_fprintf(hFile, "--------------------------------------------\n");

    tsk_fprintf(hFile, "Block Range: %" PRIuDADDR " - %" PRIuDADDR "\n",
        fs->first_block, fs->last_block);

    if (fs->last_block != fs->last_block_act)
        tsk_fprintf(hFile,
            "Total Range in Image: %" PRIuDADDR " - %" PRIuDADDR "\n",
            fs->first_block, fs->last_block_act);

    tsk_fprintf(hFile, "Block Size: %u\n", fs->block_size);

    tsk_fprintf(hFile, "Free Blocks: %" PRIu64 "\n",
        tsk_getu64(fs->endian, sb->sb_fdblocks));

    tsk_fprintf(hFile, "Sector Size: %" PRIu16 "\n",
        tsk_getu16(fs->endian, sb->sb_sectsize));

    tsk_fprintf(hFile, "\nBLOCK GROUP INFORMATION\n");
    tsk_fprintf(hFile, "--------------------------------------------\n");

    tsk_fprintf(hFile, "Number of Allocation Groups: %" PRIu32 "\n",
        tsk_getu32(fs->endian, sb->sb_agcount));

    tsk_fprintf(hFile, "Inodes per allocation group: %" PRIu64 "\n",
        inode_per_ag);
    tsk_fprintf(hFile, "Blocks per allocation group: %" PRIu32 "\n",
        tsk_getu32(fs->endian,sb->sb_agblocks));

    tsk_release_lock(&xfs->lock);

    for (i = 0; i < tsk_getu32(fs->endian,sb->sb_agcount); i++) {
        uint64_t cg_base;
        TSK_INUM_T inum;

        /* lock access to grp_buf */
        tsk_take_lock(&xfs->lock);

        // if (ext2fs_group_load(ext2fs, i)) {
        //     tsk_release_lock(&ext2fs->lock);
        //     return 1;
        // }
        tsk_fprintf(hFile, "\nAllocation Group: %d:\n", i);
        // if (ext2fs->ext4_grp_buf != NULL) {
        //     tsk_fprintf(hFile, "  Block Group Flags: [");
        //     if (EXT4BG_HAS_FLAG(fs, ext2fs->ext4_grp_buf,
        //         EXT4_BG_INODE_UNINIT))
        //         tsk_fprintf(hFile, "INODE_UNINIT, ");
        //     if (EXT4BG_HAS_FLAG(fs, ext2fs->ext4_grp_buf,
        //         EXT4_BG_BLOCK_UNINIT))
        //         tsk_fprintf(hFile, "BLOCK_UNINIT, ");
        //     if (EXT4BG_HAS_FLAG(fs, ext2fs->ext4_grp_buf,
        //         EXT4_BG_INODE_ZEROED))
        //         tsk_fprintf(hFile, "INODE_ZEROED, ");
        //     tsk_fprintf(hFile, "\b\b]\n");
        // }
        inum =
            fs->first_inum + inode_per_ag * i;
        tsk_fprintf(hFile, "  Inode Range: %" PRIuINUM " - ", inum);

        if ((inum + inode_per_ag - 1) <
            fs->last_inum)
            tsk_fprintf(hFile, "%" PRIuINUM "\n",
            inum + inode_per_ag - 1);
        else
            tsk_fprintf(hFile, "%" PRIuINUM "\n", fs->last_inum);

        tsk_release_lock(&xfs->lock);

        cg_base = xfs_cgbase_lcl(fs, sb, i);
        tsk_fprintf(hFile,
            "  Block Range: %" PRIuDADDR " - %" PRIuDADDR "\n",
            cg_base, ((xfs_cgbase_lcl(fs, sb,
            (i+1)) -1) <
            fs->last_block) ? (xfs_cgbase_lcl(fs, sb,
            (i+1)) -1) : fs->last_block);

        tsk_fprintf(hFile, "  Layout:\n");

        tsk_fprintf(hFile,
                "    Super Block: %" PRIuDADDR " - %" PRIuDADDR "\n",
                cg_base,
                cg_base +
                ((sizeof(xfs_sb) + fs->block_size -
                1) / fs->block_size) - 1);
        
        
        uint64_t rootino = tsk_getu64(fs->endian, sb->sb_rootino);
        uint16_t inodesize = tsk_getu16(fs->endian, sb->sb_inodesize);
        uint32_t blocksize = tsk_getu32(fs->endian, sb->sb_blocksize);

        tsk_fprintf(hFile, "    Root Inode Start Offset: 0x%x\n",
        rootino*inodesize+cg_base*blocksize);

        tsk_fprintf(hFile, "    Data Blocks: %lu - %lu\n",
        ((rootino+inode_per_ag)*inodesize)/blocksize+cg_base, xfs_cgbase_lcl(fs, sb, (i+1))-1);
    }

    return 0;
}

TSK_FS_INFO *
xfs_open(TSK_IMG_INFO * img_info, TSK_OFF_T offset,
    TSK_FS_TYPE_ENUM ftype, uint8_t test) {
        
    printf("hello tsk I'm mingi\n");
    printf("hi mingi im jaehoon\n");
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

    fs->ftype = TSK_FS_TYPE_XFS;
    fs->flags = 0;
    fs->img_info = img_info;
    fs->offset = offset;
    fs->tag = TSK_FS_INFO_TAG;

    /*
     * Read the superblock.
     */
    len = sizeof(xfs_sb);
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

    /*
     * Calculate the meta data info
     */
    fs->inum_count = tsk_getu64(fs->endian, xfs->fs->sb_icount) + 1;    // we are adding 1 in this calc to account for Orphans directory
    fs->last_inum = fs->inum_count;
    fs->first_inum = XFS_FIRSTINO;
    fs->root_inum = XFS_ROOTINO;

    // // 이거는 inode 개수로 xfs인지 확인하는 건데 inode개수가 최소 몇개인지 모르겠다.
    // if (fs->inum_count < 10) {
    //     fs->tag = 0;
    //     free(xfs->fs);
    //     tsk_fs_free((TSK_FS_INFO *)xfs);
    //     tsk_error_reset();
    //     tsk_error_set_errno(TSK_ERR_FS_MAGIC);
    //     tsk_error_set_errstr("Not an XFS file system (inum count)");
    //     if (tsk_verbose)
    //         fprintf(stderr, "xfs_open: two few inodes\n");
    //     return NULL;
    // }

    /* Set the size of the inode, but default to our data structure
     * size if it is larger */
    xfs->inode_size = tsk_getu16(fs->endian, xfs->fs->sb_inodesize);
    if (xfs->inode_size < sizeof(xfs_dinode_core)) {
        if (tsk_verbose)
            tsk_fprintf(stderr, "SB inode size is small");
    }
    
    
    /*
     * Calculate the block info
     */
    fs->dev_bsize = img_info->sector_size;
    fs->block_count = tsk_getu64(fs->endian, xfs->fs->sb_dblocks);
    
    fs->first_block = 0;
    fs->last_block_act = fs->last_block = fs->block_count - 1;

    fs->block_size = 0x1 << xfs->fs->sb_blocklog;

    // sanity check
    if ((fs->block_size == 0) || (fs->block_size % 512)) {
        fs->tag = 0;
        free(xfs->fs);
        tsk_fs_free((TSK_FS_INFO *)xfs);
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_MAGIC);
        tsk_error_set_errstr("Not an XFS file system (block size)");
        if (tsk_verbose)
            fprintf(stderr, "xfs_open: invalid block size\n");
        return NULL;
    }

    // determine the last block we have in this image
    if ((TSK_DADDR_T) ((img_info->size - offset) / fs->block_size) <
        fs->block_count)
        fs->last_block_act =
            (img_info->size - offset) / fs->block_size - 1;

    /* Volume ID */
    for (fs->fs_id_used = 0; fs->fs_id_used < 16; fs->fs_id_used++) {
        fs->fs_id[fs->fs_id_used] = xfs->fs->sb_uuid[fs->fs_id_used];
    }

    /* Set the generic function pointers */
    fs->inode_walk = xfs_inode_walk;
    fs->block_walk = xfs_block_walk;
    fs->block_getflags = NULL;  // TODO

    fs->get_default_attr_type = tsk_fs_unix_get_default_attr_type;
    fs->load_attrs = tsk_fs_unix_make_data_run;
    //fs->load_attrs = NULL;  // TODO

    fs->file_add_meta = xfs_inode_lookup;
    fs->dir_open_meta = xfs_dir_open_meta;
    fs->fsstat = xfs_fsstat;  // TODO
    fs->fscheck = NULL;  // TODO
    fs->istat = NULL;  // TODO
    fs->name_cmp = tsk_fs_unix_name_cmp;
    fs->close = xfs_close;


    // /* Journal */
    // fs->journ_inum = tsk_getu32(fs->endian, ext2fs->fs->s_journal_inum);
    // fs->jblk_walk = ext2fs_jblk_walk;
    // fs->jentry_walk = ext2fs_jentry_walk;
    // fs->jopen = ext2fs_jopen;

    /* initialize the caches */
    /* inode map */
    xfs->imap_buf = NULL;
    //xfs->imap_grp_num = 0xffffffff;

    /* block map */
    xfs->bmap_buf = NULL;
    //xfs->bmap_grp_num = 0xffffffff;


    /*
     * Print some stats.
     */
    if (tsk_verbose)
        tsk_fprintf(stderr,
            "inodes %" PRIu32 " root ino %" PRIuINUM " blocks %" PRIu32
            "\n", tsk_getu32(fs->endian,
                xfs->fs->sb_icount),
            fs->root_inum, tsk_getu32(fs->endian,
                xfs->fs->sb_dblocks));

    tsk_init_lock(&xfs->lock);

    return (fs);
}