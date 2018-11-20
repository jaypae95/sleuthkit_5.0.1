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
        tsk_getu64(fs->endian, &sb->s_uuid[8]), tsk_getu64(fs->endian,
            &sb->s_uuid[0]));

    tmptime = tsk_getu32(fs->endian, sb->s_wtime);
    tsk_fprintf(hFile, "\nLast Written at: %s\n",
        (tmptime > 0) ? tsk_fs_time_to_str(tmptime, timeBuf) : "empty");
    tmptime = tsk_getu32(fs->endian, sb->s_lastcheck);
    tsk_fprintf(hFile, "Last Checked at: %s\n",
        (tmptime > 0) ? tsk_fs_time_to_str(tmptime, timeBuf) : "empty");

    tmptime = tsk_getu32(fs->endian, sb->s_mtime);
    tsk_fprintf(hFile, "\nLast Mounted at: %s\n",
        (tmptime > 0) ? tsk_fs_time_to_str(tmptime, timeBuf) : "empty");

    /* State of the file system */
    if (tsk_getu16(fs->endian, sb->s_state) & EXT2FS_STATE_VALID)
        tsk_fprintf(hFile, "Unmounted properly\n");
    else
        tsk_fprintf(hFile, "Unmounted Improperly\n");

    if (sb->s_last_mounted[0] != '\0')
        tsk_fprintf(hFile, "Last mounted on: %s\n", sb->s_last_mounted);

    tsk_fprintf(hFile, "\nSource OS: ");
    switch (tsk_getu32(fs->endian, sb->s_creator_os)) {
    case EXT2FS_OS_LINUX:
        tsk_fprintf(hFile, "Linux\n");
        break;
    case EXT2FS_OS_HURD:
        tsk_fprintf(hFile, "HURD\n");
        break;
    case EXT2FS_OS_MASIX:
        tsk_fprintf(hFile, "MASIX\n");
        break;
    case EXT2FS_OS_FREEBSD:
        tsk_fprintf(hFile, "FreeBSD\n");
        break;
    case EXT2FS_OS_LITES:
        tsk_fprintf(hFile, "LITES\n");
        break;
    default:
        tsk_fprintf(hFile, "%" PRIx32 "\n", tsk_getu32(fs->endian,
                sb->s_creator_os));
        break;
    }

    if (tsk_getu32(fs->endian, sb->s_rev_level) == EXT2FS_REV_ORIG)
        tsk_fprintf(hFile, "Static Structure\n");
    else
        tsk_fprintf(hFile, "Dynamic Structure\n");


    /* add features */
    if (tsk_getu32(fs->endian, sb->s_feature_compat)) {
        tsk_fprintf(hFile, "Compat Features: ");

        if (tsk_getu32(fs->endian, sb->s_feature_compat) &
            EXT2FS_FEATURE_COMPAT_DIR_PREALLOC)
            tsk_fprintf(hFile, "Dir Prealloc, ");
        if (tsk_getu32(fs->endian, sb->s_feature_compat) &
            EXT2FS_FEATURE_COMPAT_IMAGIC_INODES)
            tsk_fprintf(hFile, "iMagic inodes, ");
        if (tsk_getu32(fs->endian, sb->s_feature_compat) &
            EXT2FS_FEATURE_COMPAT_HAS_JOURNAL)
            tsk_fprintf(hFile, "Journal, ");
        if (tsk_getu32(fs->endian, sb->s_feature_compat) &
            EXT2FS_FEATURE_COMPAT_EXT_ATTR)
            tsk_fprintf(hFile, "Ext Attributes, ");
        if (tsk_getu32(fs->endian, sb->s_feature_compat) &
            EXT2FS_FEATURE_COMPAT_RESIZE_INO)
            tsk_fprintf(hFile, "Resize Inode, ");
        if (tsk_getu32(fs->endian, sb->s_feature_compat) &
            EXT2FS_FEATURE_COMPAT_DIR_INDEX)
            tsk_fprintf(hFile, "Dir Index");

        tsk_fprintf(hFile, "\n");
    }

    if (tsk_getu32(fs->endian, sb->s_feature_incompat)) {
        tsk_fprintf(hFile, "InCompat Features: ");

        if (tsk_getu32(fs->endian, sb->s_feature_incompat) &
            EXT2FS_FEATURE_INCOMPAT_COMPRESSION)
            tsk_fprintf(hFile, "Compression, ");
        if (tsk_getu32(fs->endian, sb->s_feature_incompat) &
            EXT2FS_FEATURE_INCOMPAT_FILETYPE)
            tsk_fprintf(hFile, "Filetype, ");
        if (tsk_getu32(fs->endian, sb->s_feature_incompat) &
            EXT2FS_FEATURE_INCOMPAT_RECOVER)
            tsk_fprintf(hFile, "Needs Recovery, ");
        if (tsk_getu32(fs->endian, sb->s_feature_incompat) &
            EXT2FS_FEATURE_INCOMPAT_JOURNAL_DEV)
            tsk_fprintf(hFile, "Journal Dev");
        if (tsk_getu32(fs->endian, sb->s_feature_incompat) &
            EXT2FS_FEATURE_INCOMPAT_META_BG)
            tsk_fprintf(hFile, "Meta Block Groups, ");
        if (tsk_getu32(fs->endian, sb->s_feature_incompat) &
            EXT2FS_FEATURE_INCOMPAT_EXTENTS)
            tsk_fprintf(hFile, "Extents, ");
        if (tsk_getu32(fs->endian, sb->s_feature_incompat) &
            EXT2FS_FEATURE_INCOMPAT_64BIT)
            tsk_fprintf(hFile, "64bit, ");
        if (tsk_getu32(fs->endian, sb->s_feature_incompat) &
            EXT2FS_FEATURE_INCOMPAT_MMP)
            tsk_fprintf(hFile, "Multiple Mount Protection, ");
        if (tsk_getu32(fs->endian, sb->s_feature_incompat) &
            EXT2FS_FEATURE_INCOMPAT_FLEX_BG)
            tsk_fprintf(hFile, "Flexible Block Groups, ");
        if (tsk_getu32(fs->endian, sb->s_feature_incompat) &
            EXT2FS_FEATURE_INCOMPAT_EA_INODE)
            tsk_fprintf(hFile, "Extended Attributes, ");
        if (tsk_getu32(fs->endian, sb->s_feature_incompat) &
            EXT2FS_FEATURE_INCOMPAT_DIRDATA)
            tsk_fprintf(hFile, "Directory Entry Data");

        tsk_fprintf(hFile, "\n");
    }

    if (tsk_getu32(fs->endian, sb->s_feature_ro_compat)) {
        tsk_fprintf(hFile, "Read Only Compat Features: ");

        if (tsk_getu32(fs->endian, sb->s_feature_ro_compat) &
            EXT2FS_FEATURE_RO_COMPAT_SPARSE_SUPER)
            tsk_fprintf(hFile, "Sparse Super, ");
        if (tsk_getu32(fs->endian, sb->s_feature_ro_compat) &
            EXT2FS_FEATURE_RO_COMPAT_LARGE_FILE)
            tsk_fprintf(hFile, "Large File, ");
        if (EXT2FS_HAS_RO_COMPAT_FEATURE(fs, sb,
                EXT2FS_FEATURE_RO_COMPAT_HUGE_FILE))
            tsk_fprintf(hFile, "Huge File, ");
        if (tsk_getu32(fs->endian, sb->s_feature_ro_compat) &
            EXT2FS_FEATURE_RO_COMPAT_BTREE_DIR)
            tsk_fprintf(hFile, "Btree Dir, ");
        if (tsk_getu32(fs->endian, sb->s_feature_ro_compat) &
            EXT2FS_FEATURE_RO_COMPAT_EXTRA_ISIZE)
            tsk_fprintf(hFile, "Extra Inode Size");

        tsk_fprintf(hFile, "\n");
    }

    /* Print journal information */
    if (tsk_getu32(fs->endian, sb->s_feature_compat) &
        EXT2FS_FEATURE_COMPAT_HAS_JOURNAL) {

        tsk_fprintf(hFile, "\nJournal ID: %" PRIx64 "%" PRIx64 "\n",
            tsk_getu64(fs->endian, &sb->s_journal_uuid[8]),
            tsk_getu64(fs->endian, &sb->s_journal_uuid[0]));

        if (tsk_getu32(fs->endian, sb->s_journal_inum) != 0)
            tsk_fprintf(hFile, "Journal Inode: %" PRIu32 "\n",
                tsk_getu32(fs->endian, sb->s_journal_inum));

        if (tsk_getu32(fs->endian, sb->s_journal_dev) != 0)
            tsk_fprintf(hFile, "Journal Device: %" PRIu32 "\n",
                tsk_getu32(fs->endian, sb->s_journal_dev));


    }

    tsk_fprintf(hFile, "\nMETADATA INFORMATION\n");
    tsk_fprintf(hFile, "--------------------------------------------\n");

    tsk_fprintf(hFile, "Inode Range: %" PRIuINUM " - %" PRIuINUM "\n",
        fs->first_inum, fs->last_inum);
    tsk_fprintf(hFile, "Root Directory: %" PRIuINUM "\n", fs->root_inum);

    tsk_fprintf(hFile, "Free Inodes: %" PRIu32 "\n",
        tsk_getu32(fs->endian, sb->s_free_inode_count));
    /*
       Only print size of inode for Ext4
       This determines if you will get nanosecs and crtime
     */
    if (!strcmp(tmptypename, "Ext4")) {
        tsk_fprintf(hFile, "Inode Size: %" PRIu16 "\n",
            tsk_getu16(fs->endian, sb->s_inode_size));
    }


    if (tsk_getu32(fs->endian, sb->s_last_orphan)) {
        uint32_t or_in;
        tsk_fprintf(hFile, "Orphan Inodes: ");
        or_in = tsk_getu32(fs->endian, sb->s_last_orphan);

        while (or_in) {
            TSK_FS_FILE *fs_file;

            if ((or_in > fs->last_inum) || (or_in < fs->first_inum))
                break;

            tsk_fprintf(hFile, "%" PRIu32 ", ", or_in);

            if ((fs_file = tsk_fs_file_alloc(fs)) == NULL) {
                /* Ignore this error */
                tsk_error_reset();
                break;
            }

            /* Get the next one */
            if (ext2fs_inode_lookup(fs, fs_file, or_in)) {
                /* Ignore this error */
                tsk_error_reset();
                break;
            }

            or_in = (uint32_t) fs_file->meta->time2.ext2.dtime;
            tsk_fs_file_close(fs_file);
        }
        tsk_fprintf(hFile, "\n");
    }

    tsk_fprintf(hFile, "\nCONTENT INFORMATION\n");
    tsk_fprintf(hFile, "--------------------------------------------\n");

    if (fs->ftype == TSK_FS_TYPE_EXT4) {
        tsk_fprintf(hFile, "Block Groups Per Flex Group: %" PRIu32 "\n",
            (1 << sb->s_log_groups_per_flex));
//        gpfbg = (1 << sb->s_log_groups_per_flex);
    }

    tsk_fprintf(hFile, "Block Range: %" PRIuDADDR " - %" PRIuDADDR "\n",
        fs->first_block, fs->last_block);

    if (fs->last_block != fs->last_block_act)
        tsk_fprintf(hFile,
            "Total Range in Image: %" PRIuDADDR " - %" PRIuDADDR "\n",
            fs->first_block, fs->last_block_act);

    tsk_fprintf(hFile, "Block Size: %u\n", fs->block_size);

    if (tsk_getu32(fs->endian, sb->s_first_data_block))
        tsk_fprintf(hFile,
            "Reserved Blocks Before Block Groups: %" PRIu32 "\n",
            tsk_getu32(fs->endian, sb->s_first_data_block));

    tsk_fprintf(hFile, "Free Blocks: %" PRIu32 "\n",
        tsk_getu32(fs->endian, sb->s_free_blocks_count));

    tsk_fprintf(hFile, "\nBLOCK GROUP INFORMATION\n");
    tsk_fprintf(hFile, "--------------------------------------------\n");

    tsk_fprintf(hFile, "Number of Block Groups: %" PRI_EXT2GRP "\n",
        ext2fs->groups_count);

    tsk_fprintf(hFile, "Inodes per group: %" PRIu32 "\n",
        tsk_getu32(fs->endian, sb->s_inodes_per_group));
    tsk_fprintf(hFile, "Blocks per group: %" PRIu32 "\n",
        tsk_getu32(fs->endian, sb->s_blocks_per_group));


    /* number of blocks the inodes consume */
    ibpg =
        (tsk_getu32(fs->endian,
            sb->s_inodes_per_group) * ext2fs->inode_size + fs->block_size -
        1) / fs->block_size;
    /* number of blocks group descriptors consume */
//    gd_blocks =
//        (unsigned int)((gd_size * ext2fs->groups_count + fs->block_size -
//        1) / fs->block_size);

#ifdef Ext4_DBG
    tsk_fprintf(hFile, "\n\tDEBUG: Group Descriptor Size: %d\n", gd_size);      //DEBUG
    tsk_fprintf(hFile, "\n\tDEBUG: Group Descriptor Size: %d\n", *sb->s_desc_size);     //DEBUG
    debug_print_buf((unsigned char *) &sb->pad_or_gdt, 16);
    printf("\n\tDEBUG: gdt_growth: %d\n", tsk_getu16(fs->endian,
        sb->pad_or_gdt.s_reserved_gdt_blocks));
#endif

    for (i = 0; i < ext2fs->groups_count; i++) {
        TSK_DADDR_T cg_base;
        TSK_INUM_T inum;

        /* lock access to grp_buf */
        tsk_take_lock(&ext2fs->lock);

        if (ext2fs_group_load(ext2fs, i)) {
            tsk_release_lock(&ext2fs->lock);
            return 1;
        }
        tsk_fprintf(hFile, "\nGroup: %d:\n", i);
        if (ext2fs->ext4_grp_buf != NULL) {
            tsk_fprintf(hFile, "  Block Group Flags: [");
            if (EXT4BG_HAS_FLAG(fs, ext2fs->ext4_grp_buf,
                EXT4_BG_INODE_UNINIT))
                tsk_fprintf(hFile, "INODE_UNINIT, ");
            if (EXT4BG_HAS_FLAG(fs, ext2fs->ext4_grp_buf,
                EXT4_BG_BLOCK_UNINIT))
                tsk_fprintf(hFile, "BLOCK_UNINIT, ");
            if (EXT4BG_HAS_FLAG(fs, ext2fs->ext4_grp_buf,
                EXT4_BG_INODE_ZEROED))
                tsk_fprintf(hFile, "INODE_ZEROED, ");
            tsk_fprintf(hFile, "\b\b]\n");
        }
        inum =
            fs->first_inum + tsk_gets32(fs->endian,
            sb->s_inodes_per_group) * i;
        tsk_fprintf(hFile, "  Inode Range: %" PRIuINUM " - ", inum);

        if ((inum + tsk_gets32(fs->endian, sb->s_inodes_per_group) - 1) <
            fs->last_inum)
            tsk_fprintf(hFile, "%" PRIuINUM "\n",
            inum + tsk_gets32(fs->endian, sb->s_inodes_per_group) - 1);
        else
            tsk_fprintf(hFile, "%" PRIuINUM "\n", fs->last_inum);

        if (tsk_getu32(fs->endian,
            ext2fs->fs->
            s_feature_incompat) & EXT2FS_FEATURE_INCOMPAT_64BIT) {
                cg_base = ext4_cgbase_lcl(fs, sb, i);
#ifdef Ext4_DBG
                printf("DEBUG64: ext2_cgbase_lcl %" PRIuDADDR "\n", cg_base);
                printf("DEBUG64: fs->s_first_data_block %" PRIuDADDR "\n",
                    tsk_getu32(fs->endian, sb->s_first_data_block));
                printf("DEBUG64: blocks_per_group %" PRIuDADDR "\n",
                    tsk_getu32(fs->endian, sb->s_blocks_per_group));
                printf("DEBUG64: i %" PRIuDADDR " %" PRIuDADDR " %" PRIuDADDR
                    "\n", i, tsk_getu32(fs->endian, sb->s_blocks_per_group),
                    (uint64_t) i * (uint64_t) tsk_getu32(fs->endian,
                    sb->s_blocks_per_group));
                //printf("DEBUG: calculated %"PRIuDADDR"\n", )
#endif
                tsk_fprintf(hFile,
                    "  Block Range: %" PRIuDADDR " - %" PRIuDADDR "\n",
                    cg_base, ((ext4_cgbase_lcl(fs, sb,
                    i + 1) - 1) <
                    fs->last_block) ? (ext4_cgbase_lcl(fs, sb,
                    i + 1) - 1) : fs->last_block);
        }
        else {
            cg_base = ext2_cgbase_lcl(fs, sb, i);
#ifdef Ext4_DBG
            debug_print_buf(sb, 100);
            printf("DEBUG32: ext2_cgbase_lcl %" PRIuDADDR "\n", cg_base);
            printf("DEBUG32: fs->s_first_data_block %" PRIu32 "\n",
                tsk_getu32(fs->endian, sb->s_first_data_block));
            printf("DEBUG32: blocks_per_group %" PRIu32 "\n",
                tsk_getu32(fs->endian, sb->s_blocks_per_group));
            printf("DEBUG32: i: %" PRIu32 " blocks per group: %" PRIu32
                " i*blocks_per_group: %" PRIu32 "\n",
                i, tsk_getu32(fs->endian, sb->s_blocks_per_group),
                (uint64_t) i * (uint64_t) tsk_getu32(fs->endian,
                sb->s_blocks_per_group));
            //printf("DEBUG: calculated %"PRIuDADDR"\n", )
#endif
            tsk_fprintf(hFile,
                "  Block Range: %" PRIuDADDR " - %" PRIuDADDR "\n",
                cg_base, ((ext2_cgbase_lcl(fs, sb,
                i + 1) - 1) <
                fs->last_block) ? (ext2_cgbase_lcl(fs, sb,
                i + 1) - 1) : fs->last_block);
        }



        tsk_fprintf(hFile, "  Layout:\n");

        /* only print the super block data if we are not in a sparse
        * group
        */
#ifdef Ext4_DBG
        printf("DEBUG: ext2fs_super: %d\n",
            ext2fs_bg_has_super(tsk_getu32(fs->endian,
            sb->s_feature_ro_compat), i));
#endif
        /*        if (((tsk_getu32(fs->endian, ext2fs->fs->s_feature_ro_compat) &
        EXT2FS_FEATURE_RO_COMPAT_SPARSE_SUPER) &&
        (cg_base != tsk_getu32(fs->endian,
        ext2fs->grp_buf->bg_block_bitmap)))
        || ((tsk_getu32(fs->endian,
        ext2fs->fs->s_feature_ro_compat) &
        EXT2FS_FEATURE_RO_COMPAT_SPARSE_SUPER) == 0)) {
        */
        if (ext2fs_bg_has_super(tsk_getu32(fs->endian,
            sb->s_feature_ro_compat), i)) {
                TSK_OFF_T boff;

                /* the super block is the first 1024 bytes */
                tsk_fprintf(hFile,
                    "    Super Block: %" PRIuDADDR " - %" PRIuDADDR "\n",
                    cg_base,
                    cg_base +
                    ((sizeof(ext2fs_sb) + fs->block_size -
                    1) / fs->block_size) - 1);

                boff = roundup(sizeof(ext2fs_sb), fs->block_size);

                /* Group Descriptors */
                tsk_fprintf(hFile,
                    "    Group Descriptor Table: %" PRIuDADDR " - ",
                    (cg_base + (boff + fs->block_size - 1) / fs->block_size));

                //            printf("DEBUG: Groups Count: %u * gd_size: %u = %u\n", ext2fs->groups_count, gd_size, ext2fs->groups_count * gd_size);
                boff += (ext2fs->groups_count * gd_size);
                tsk_fprintf(hFile, "%" PRIuDADDR "\n",
                    ((cg_base +
                    (boff + fs->block_size - 1) / fs->block_size) -
                    1));
                if (fs->ftype == TSK_FS_TYPE_EXT4) {
                    tsk_fprintf(hFile,
                        "    Group Descriptor Growth Blocks: %" PRIuDADDR
                        " - ",
                        cg_base + (boff + fs->block_size -
                        1) / fs->block_size);
                    boff +=
                        tsk_getu16(fs->endian,
                        ext2fs->fs->pad_or_gdt.s_reserved_gdt_blocks) *
                        fs->block_size;
                    tsk_fprintf(hFile, "%" PRIuDADDR "\n",
                        ((cg_base + (boff + fs->block_size -
                        1) / fs->block_size) - 1));
                }
        }


        if (ext2fs->ext4_grp_buf != NULL) {
            /* The block bitmap is a full block */
            tsk_fprintf(hFile,
                "    Data bitmap: %" PRIu64 " - %" PRIu64 "\n",
                ext4_getu64(fs->endian,
                ext2fs->ext4_grp_buf->bg_block_bitmap_hi,
                ext2fs->ext4_grp_buf->bg_block_bitmap_lo),
                ext4_getu64(fs->endian,
                ext2fs->ext4_grp_buf->bg_block_bitmap_hi,
                ext2fs->ext4_grp_buf->bg_block_bitmap_lo));


            /* The inode bitmap is a full block */
            tsk_fprintf(hFile,
                "    Inode bitmap: %" PRIu64 " - %" PRIu64 "\n",
                ext4_getu64(fs->endian,
                ext2fs->ext4_grp_buf->bg_inode_bitmap_hi,
                ext2fs->ext4_grp_buf->bg_inode_bitmap_lo),
                ext4_getu64(fs->endian,
                ext2fs->ext4_grp_buf->bg_inode_bitmap_hi,
                ext2fs->ext4_grp_buf->bg_inode_bitmap_lo));


            tsk_fprintf(hFile,
                "    Inode Table: %" PRIu64 " - %" PRIu64 "\n",
                ext4_getu64(fs->endian,
                ext2fs->ext4_grp_buf->bg_inode_table_hi,
                ext2fs->ext4_grp_buf->bg_inode_table_lo),
                ext4_getu64(fs->endian,
                ext2fs->ext4_grp_buf->bg_inode_table_hi,
                ext2fs->ext4_grp_buf->bg_inode_table_lo)
                + ibpg - 1);

            ext4_fsstat_datablock_helper(fs, hFile, i, cg_base, gd_size);
        }
        else {
            /* The block bitmap is a full block */
            tsk_fprintf(hFile,
                "    Data bitmap: %" PRIu32 " - %" PRIu32 "\n",
                tsk_getu32(fs->endian, ext2fs->grp_buf->bg_block_bitmap),
                tsk_getu32(fs->endian, ext2fs->grp_buf->bg_block_bitmap));


            /* The inode bitmap is a full block */
            tsk_fprintf(hFile,
                "    Inode bitmap: %" PRIu32 " - %" PRIu32 "\n",
                tsk_getu32(fs->endian, ext2fs->grp_buf->bg_inode_bitmap),
                tsk_getu32(fs->endian, ext2fs->grp_buf->bg_inode_bitmap));


            tsk_fprintf(hFile,
                "    Inode Table: %" PRIu32 " - %" PRIu32 "\n",
                tsk_getu32(fs->endian, ext2fs->grp_buf->bg_inode_table),
                tsk_getu32(fs->endian,
                ext2fs->grp_buf->bg_inode_table) + ibpg - 1);
        
            tsk_fprintf(hFile, "    Data Blocks: ");
            // BC: Commented out from Ext4 commit because it produced
            // bad data on Ext2 test image.
            //if (ext2fs_bg_has_super(tsk_getu32(fs->endian,
            //            sb->s_feature_ro_compat), i)) {
            if ((tsk_getu32(fs->endian, ext2fs->fs->s_feature_ro_compat) &
                EXT2FS_FEATURE_RO_COMPAT_SPARSE_SUPER) &&
                (cg_base == tsk_getu32(fs->endian,
                ext2fs->grp_buf->bg_block_bitmap))) {

                    /* it goes from the end of the inode bitmap to before the
                    * table
                    *
                    * This hard coded aspect does not scale ...
                    */

                    tsk_fprintf(hFile, "%" PRIu32 " - %" PRIu32 ", ",
                        tsk_getu32(fs->endian,
                        ext2fs->grp_buf->bg_inode_bitmap) + 1,
                        tsk_getu32(fs->endian,
                        ext2fs->grp_buf->bg_inode_table) - 1);
            }

            tsk_fprintf(hFile, "%" PRIu32 " - %" PRIu32 "\n",
                (uint64_t) tsk_getu32(fs->endian,
                ext2fs->grp_buf->bg_inode_table) + ibpg,
                ((ext2_cgbase_lcl(fs, sb, i + 1) - 1) <
                fs->last_block) ? (ext2_cgbase_lcl(fs, sb,
                i + 1) - 1) : fs->last_block);
        }

        /* Print the free info */

        /* The last group may not have a full number of blocks */
        if (i != (ext2fs->groups_count - 1)) {
            uint64_t tmpInt;

            if (ext2fs->ext4_grp_buf != NULL) 
                // @@@ Should be 32-bit
                tmpInt = tsk_getu16(fs->endian,
                    ext2fs->ext4_grp_buf->bg_free_inodes_count_lo);
            else
                tmpInt = tsk_getu16(fs->endian,
                    ext2fs->grp_buf->bg_free_inodes_count);
            
            tsk_fprintf(hFile,
                "  Free Inodes: %" PRIu32 " (%" PRIu32 "%%)\n",
                tmpInt, (100 * tmpInt) /  
                tsk_getu32(fs->endian, sb->s_inodes_per_group));


            if (ext2fs->ext4_grp_buf != NULL) 
                // @@@ Should be 32-bit
                tmpInt = tsk_getu16(fs->endian,
                    ext2fs->ext4_grp_buf->bg_free_blocks_count_lo);
            else
                tmpInt = tsk_getu16(fs->endian,
                    ext2fs->grp_buf->bg_free_blocks_count);

            tsk_fprintf(hFile,
                "  Free Blocks: %" PRIu32 " (%" PRIu32 "%%)\n",
                tmpInt,
                (100 * tmpInt) / 
                tsk_getu32(fs->endian, sb->s_blocks_per_group));
        }
        else {
            TSK_INUM_T inum_left;
            TSK_DADDR_T blk_left;
            uint64_t tmpInt;

            inum_left =
                (fs->last_inum % tsk_gets32(fs->endian,
                sb->s_inodes_per_group)) - 1;

            if (inum_left == 0)
                inum_left = tsk_getu32(fs->endian, sb->s_inodes_per_group);

            if (ext2fs->ext4_grp_buf != NULL) 
                // @@@ Should be 32-bit
                tmpInt = tsk_getu16(fs->endian,
                    ext2fs->ext4_grp_buf->bg_free_inodes_count_lo);
            else
                tmpInt = tsk_getu16(fs->endian,
                    ext2fs->grp_buf->bg_free_inodes_count);
            
            tsk_fprintf(hFile, "  Free Inodes: %" PRIu32 " (%d%%)\n",
                tmpInt, 100 * tmpInt / inum_left); 

            /* Now blocks */
            blk_left =
                fs->block_count % tsk_getu32(fs->endian,
                sb->s_blocks_per_group);
            if (blk_left == 0)
                blk_left = tsk_getu32(fs->endian, sb->s_blocks_per_group);

            if (ext2fs->ext4_grp_buf != NULL) 
                // @@@ Should be 32-bit
                tmpInt = tsk_getu16(fs->endian,
                    ext2fs->ext4_grp_buf->bg_free_blocks_count_lo);
            else
                tmpInt = tsk_getu16(fs->endian,
                    ext2fs->grp_buf->bg_free_blocks_count);

            tsk_fprintf(hFile, "  Free Blocks: %" PRIu32 " (%d%%)\n",
                tmpInt, 100 * tmpInt / blk_left);
        }


        if (ext2fs->ext4_grp_buf != NULL) {
            // @@@@ Sould be 32-bit
            tsk_fprintf(hFile, "  Total Directories: %" PRIu16 "\n",
                tsk_getu16(fs->endian, ext2fs->ext4_grp_buf->bg_used_dirs_count_lo));

            tsk_fprintf(hFile, "  Stored Checksum: 0x%04" PRIX16 "\n",
                tsk_getu16(fs->endian, ext2fs->ext4_grp_buf->bg_checksum));
#ifdef EXT4_CHECKSUMS
            //Need Non-GPL CRC16
            tsk_fprintf(hFile, "  Calculated Checksum: 0x%04" PRIX16 "\n",
                ext4_group_desc_csum(ext2fs->fs, i, ext2fs->ext4_grp_buf));
#endif
        }
        else {
            tsk_fprintf(hFile, "  Total Directories: %" PRIu16 "\n",
               tsk_getu16(fs->endian, ext2fs->grp_buf->bg_used_dirs_count));
        }

        tsk_release_lock(&ext2fs->lock);
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
    //fs->load_attrs = tsk_fs_unix_make_data_run;
    fs->load_attrs = NULL;  // TODO

    fs->file_add_meta = NULL;  // TODO
    fs->dir_open_meta = NULL;  // TODO
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