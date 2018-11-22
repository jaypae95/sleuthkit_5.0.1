#include "tsk_fs_i.h"
#include "tsk_xfs.h"

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
    XFS_INFO *xfs = (EXT2FS_INFO *) fs;
    xfs_dinode_core *dino_core_buf = NULL;
    unsigned int size = 0;

    if (a_fs_file == NULL) {
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr("xfs_inode_lookup: fs_file is NULL");
        return 1;
    }

    if (a_fs_file->meta == NULL) {
        a_fs_file->meta = tsk_fs_meta_alloc(HFS_FILE_CONTENT_LEN);
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
        ext2fs->inode_size >
        sizeof(ext2fs_inode) ? ext2fs->inode_size : sizeof(ext2fs_inode);
    if ((dino_buf = (ext2fs_inode *) tsk_malloc(size)) == NULL) {
        return 1;
    }

    if (ext2fs_dinode_load(ext2fs, inum, dino_buf)) {
        free(dino_buf);
        return 1;
    }

    if (ext2fs_dinode_copy(ext2fs, a_fs_file->meta, inum, dino_buf)) {
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


    // clean up any error messages that are lying around
    tsk_error_reset();

    tsk_fprintf(hFile, "FILE SYSTEM INFORMATION\n");
    tsk_fprintf(hFile, "--------------------------------------------\n");

    tsk_fprintf(hFile, "File System Type: %s\n", "XFS");
    tsk_fprintf(hFile, "Volume Name: %s\n", sb->sb_fname);
    tsk_fprintf(hFile, "Volume ID: %" PRIx64 "%" PRIx64 "\n",
        tsk_getu64(fs->endian, &sb->sb_uuid[8]), tsk_getu64(fs->endian,
            &sb->sb_uuid[0]));

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

    tsk_fprintf(hFile, "\nCONTENT INFORMATION\n");
    tsk_fprintf(hFile, "--------------------------------------------\n");

    tsk_fprintf(hFile, "Block Range: %" PRIuDADDR " - %" PRIuDADDR "\n",
        fs->first_block, fs->last_block);

    if (fs->last_block != fs->last_block_act)
        tsk_fprintf(hFile,
            "Total Range in Image: %" PRIuDADDR " - %" PRIuDADDR "\n",
            fs->first_block, fs->last_block_act);

    tsk_fprintf(hFile, "Block Size: %u\n", fs->block_size);

    tsk_fprintf(hFile, "Number of Free Blocks: %" PRIu32 "\n",
        tsk_getu32(fs->endian, sb->sb_ifree));

    tsk_release_lock(&xfs->lock);

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
    //fs->load_attrs = tsk_fs_unix_make_data_run;
    fs->load_attrs = NULL;  // TODO

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