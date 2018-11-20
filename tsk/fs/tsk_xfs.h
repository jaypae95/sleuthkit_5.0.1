#ifndef _TSK_XFS_H
#define _TSK_XFS_H

#include <stdint.h>
#include <tsk_fs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XFS_FIRSTINO    0x1
#define XFS_ROOTINO     0x2
#define XFS_SBOFF       0x0
#define XFS_FS_MAGIC    0x58465342

    typedef struct xfs_dinode_core {
        uint8_t           di_magic[2];
        uint8_t           di_mode[2];
        uint8_t           di_version;
        uint8_t           di_format;
        uint8_t           di_onlink[2];
        uint8_t           di_uid[4];
        uint8_t           di_gid[4];
        uint8_t           di_nlink[4];
        uint8_t           di_projid[2];
        uint8_t           di_pad[8];
        uint8_t           di_flushiter[2];
        uint8_t           di_atime[4];
        uint8_t           di_mtime[4];
        uint8_t           di_ctime[4];
        uint8_t           di_size[8];
        uint8_t           di_nblocks[8];
        uint8_t           di_extsize[4];
        uint8_t           di_nextents[4];
        uint8_t           di_anextents[2];
        uint8_t           di_forkoff;
        uint8_t           di_aformat;
        uint8_t           di_dmevmask[4];
        uint8_t           di_dmstate[2];
        uint8_t           di_flags[2];
        uint8_t           di_gen[4];
    } xfs_dinode_core;

    typedef struct {
        uint8_t sb_magicnum[4];     /* u32 magic_number */
        uint8_t sb_blocksize[4];     /* u32 block size */
        uint8_t sb_dblocks[8];     /* u32 block count */
        uint8_t sb_rblocks[8];     /* u32 Number blocks in the real-time disk device. */
        uint8_t sb_rextents[8];     /* u32 Number of extents on the real-time device. */
        uint8_t sb_uuid[16];     /* u32 UUID */
        uint8_t sb_logstart[8];     /* u32 */
        uint8_t sb_rootino[8];     /* u32 */
        uint8_t sb_rbmino[8];     /* u32 */
        uint8_t sb_rsumino[8];     /* u32 */
        uint8_t sb_rextsize[8];     /* u32 */
        uint8_t sb_agblocks[4];
        uint8_t sb_agcount[4];
        uint8_t sb_rbmblocks[4];
        uint8_t sb_logblocks[4];
        uint8_t sb_versionnum[2];
        uint8_t sb_sectsize[2];
        uint8_t sb_inodesize[2];
        uint8_t sb_inopblock[2];
        char    sb_fname[12];
        uint8_t sb_blocklog;
        uint8_t sb_sectlog;
        uint8_t sb_inodelog;
        uint8_t sb_inopblog;
        uint8_t sb_agblklog;
        uint8_t sb_rextslog;
        uint8_t sb_inprogress;
        uint8_t sb_imax_pct;
        uint8_t sb_icount[8];
        uint8_t sb_ifree[8];
        uint8_t sb_fdblocks[8];
        uint8_t sb_frextents[8];
        uint8_t sb_uquotino[4];
        uint8_t sb_gquotino[4];
        uint8_t sb_qflags[2];
        uint8_t sb_flags;
        uint8_t sb_shared_vn;
        uint8_t sb_inoalignmt[4];
        uint8_t sb_unit[4];
        uint8_t sb_width[4];
        uint8_t sb_dirblklog;
        uint8_t sb_logsectlog;
        uint8_t sb_logsectsize[2];
        uint8_t sb_logsunit[4];
        uint8_t sb_features2[4];
    } xfs_sb;

    /*
     * Structure of an ext2fs file system handle.
     */
    typedef struct {
        TSK_FS_INFO fs_info;    /* super class */
        xfs_sb *fs;          /* super block */

        /* lock protects grp_buf, grp_num, bmap_buf, bmap_grp_num, imap_buf, imap_grp_num */
        tsk_lock_t lock;

        // one of the below will be allocated and populated by ext2fs_group_load depending on the FS type
//        ext4fs_gd *ext4_grp_buf; /* cached group descriptor for 64-bit ext4 r/w shared - lock */
//        ext2fs_gd *grp_buf;     /* cached group descriptor for ext2,ext3,32-bit ext4 r/w shared - lock */
//
//        EXT2_GRPNUM_T grp_num;  /* cached group number r/w shared - lock */
//
        uint8_t *bmap_buf;      /* cached block allocation bitmap r/w shared - lock */
//        EXT2_GRPNUM_T bmap_grp_num;     /* cached block bitmap nr r/w shared - lock */
//
        uint8_t *imap_buf;      /* cached inode allocation bitmap r/w shared - lock */
//        EXT2_GRPNUM_T imap_grp_num;     /* cached inode bitmap nr r/w shared - lock */
//
//        TSK_OFF_T groups_offset;        /* offset to first group desc */
//        EXT2_GRPNUM_T groups_count;     /* nr of descriptor group blocks */
//        uint8_t deentry_type;   /* v1 or v2 of dentry */
        uint16_t inode_size;    /* size of each inode */
//        TSK_DADDR_T first_data_block;
//
//        EXT2FS_JINFO *jinfo;
    } XFS_INFO;

#ifdef __cplusplus
}
#endif
#endif