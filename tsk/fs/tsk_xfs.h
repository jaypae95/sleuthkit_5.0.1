#ifndef _TSK_XFS_H
#define _TSK_XFS_H

#include <stdint.h>
#include <tsk_fs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XFS_EXTENT_MAX 21
#define XFS_DIR2_DATA_FD_COUNT 3
//add
#define PRI_XFSGRP	PRIu64

#define XFS_IN_FMT  0170000
#define XFS_IN_SOCK 0140000
#define XFS_IN_LNK  0120000
#define XFS_IN_REG  0100000
#define XFS_IN_BLK  0060000
#define XFS_IN_DIR  0x41ed
#define XFS_IN_CHR  0020000
#define XFS_IN_FIFO  0010000

/* xfs directory file types  */
#define XFS_DE_UNKNOWN         0
#define XFS_DE_REG        1
#define XFS_DE_DIR             2
#define XFS_DE_CHR          3
#define XFS_DE_BLK          4
#define XFS_DE_FIFO            5
#define XFS_DE_SOCK            6
#define XFS_DE_LNK         7
#define XFS_DE_MAX             8

#define XFS_FIRSTINO    0x1
#define XFS_ROOTINO     0x1
#define XFS_SBOFF       0x0
#define XFS_FS_MAGIC    0x58465342
#define XFS_MIN_BLOCK_SIZE	512
#define XFS_MAX_BLOCK_SIZE	65536
#define XFS_FILE_CONTENT_LEN  20*sizeof(TSK_DADDR_T)

#define XFS_MAXNAMLEN 255

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
        uint8_t           di_atime[8];
        uint8_t           di_mtime[8];
        uint8_t           di_ctime[8];
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

        /* di_next_unlinked is the only non-core field in the old dinode */ 
        uint8_t di_next_unlinked[4]; 
        
        /* version 5 filesystem (inode version 3) fields start here */
        uint8_t di_crc[4];
        uint8_t di_changecount[8];
        uint8_t di_lsn[8];
        uint8_t di_flags2[8];
        uint8_t di_cowextsize[4];
        uint8_t di_pad2[12];
        uint8_t di_crtime[8];
        uint8_t di_ino[8];
        uint8_t di_uuid[16];
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
        uint8_t sb_rextsize[4];     /* u32 */
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
        //add
        uint8_t sb_badfeatures2[4];
        uint8_t sb_features_cmpt[4];
        uint8_t sb_features_rocmpt[4];
        uint8_t sb_features_incmpt[4];
        uint8_t sb_checksum[4];
        uint8_t sb_sa_extlen[8];
        uint8_t sb_pqinode[8];
        uint8_t sb_lsnum[8];
        uint8_t sb_meta_uuid[16];
        uint8_t sb_rrmapino[8];

    } xfs_sb;

    typedef __uint16_t xfs_dir2_data_off_t;
    typedef __uint32_t xfs_dir2_dataptr_t;
    typedef __uint64_t xfs_ino_t;

    typedef struct xfs_dir2_block_tail
    {
        __uint32_t count;
        __uint32_t stale;
    } xfs_dir2_block_tail_t;

    typedef struct xfs_dir2_leaf_entry
    {
        uint32_t hashval;
        xfs_dir2_dataptr_t address;
    } xfs_dir2_leaf_entry_t;

    typedef struct xfs_dir2_data_unused
    {
        __uint16_t freetag; /* 0xffff */
        xfs_dir2_data_off_t length;
        xfs_dir2_data_off_t tag;
    } xfs_dir2_data_unused_t;

    typedef struct xfs_dir2_data_entry
    {
        xfs_ino_t inumber;
        __uint8_t namelen;
        __uint8_t name[1];
        __uint8_t ftype;
        xfs_dir2_data_off_t tag;
    } xfs_dir2_data_entry_t;

    typedef union {
        xfs_dir2_data_entry_t entry;
        xfs_dir2_data_unused_t unused;
    } xfs_dir2_data_union_t;

    typedef struct xfs_dir2_data_free
    {
        xfs_dir2_data_off_t offset;
        xfs_dir2_data_off_t length;
    } xfs_dir2_data_free_t;

    struct xfs_dir3_data_hdr
    {
        struct xfs_dir3_blk_hdr hdr;
        xfs_dir2_data_free_t best_free[XFS_DIR2_DATA_FD_COUNT];
        uint32_t pad;
    };

    struct xfs_dir3_blk_hdr
    {
        uint32_t magic;
        uint32_t crc;
        uint64_t blkno;
        uint64_t lsn;
        uint8_t di_uuid[16];
        uint64_t owner;
    };

    typedef struct xfs_dir2_data_hdr
    {
        __uint32_t magic;
        xfs_dir2_data_free_t bestfree[XFS_DIR2_DATA_FD_COUNT];
    } xfs_dir2_data_hdr_t;
    
    typedef struct xfs_dir2_block
    {
        xfs_dir2_data_hdr_t hdr;
        xfs_dir2_data_union_t u[1];
        xfs_dir2_leaf_entry_t leaf[1];
        xfs_dir2_block_tail_t tail;
    } xfs_dir2_block_t;

    /*
 * Bmap root header, on-disk form only.
 */
    typedef struct xfs_bmdr_block
    {
        uint16_t bb_level;   /* 0 is a leaf */
        uint16_t bb_numrecs; /* current # of data records */
} xfs_bmdr_block_t;

typedef struct xfs_bmbt_rec_32
{
	uint32_t		l0, l1, l2, l3;
} xfs_bmbt_rec_32_t;

typedef struct { uint8_t i[8]; } xfs_dir2_ino8_t;
typedef struct { uint8_t i[4]; } xfs_dir2_ino4_t;
typedef union {
    xfs_dir2_ino8_t i8;
    xfs_dir2_ino4_t i4;
} xfs_dir2_inou_t;

typedef struct xfs_dir2_sf_entry {
    uint8_t namelen;
    uint8_t offset[2];
    char name[XFS_MAXNAMLEN];
    uint8_t ftype;
    xfs_dir2_inou_t inumber;
} xfs_dir2_sf_entry_t;

typedef struct xfs_dir2_sf_hdr {
    uint8_t count;
    uint8_t i8count;
    xfs_dir2_inou_t parent;
} xfs_dir2_sf_hdr_t;

typedef struct xfs_dir2_sf {
    xfs_dir2_sf_hdr_t hdr;
    xfs_dir2_sf_entry_t list[XFS_EXTENT_MAX];
} xfs_dir2_sf_t;

typedef struct xfs_attr_shortform {
    struct xfs_attr_sf_hdr {
        uint16_t totsize;
        uint8_t count;
    } hdr;

    struct xfs_attr_sf_entry {
        uint8_t namelen;
        uint8_t valuelen;
        uint8_t flags;
        uint8_t nameval[1];
    } list[1];
} xfs_attr_shortform_t;

typedef struct xfs_dinode
{
	xfs_dinode_core di_core;
	/*
	 * In adding anything between the core and the union, be
	 * sure to update the macros like XFS_LITINO below and
	 * XFS_BMAP_RBLOCK_DSIZE in xfs_bmap_btree.h.
	 */
	uint32_t di_next_unlinked;/* agi unlinked list ptr */
	union {
		xfs_bmdr_block_t di_bmbt;	/* btree root block */
		xfs_bmbt_rec_32_t di_bmx[XFS_EXTENT_MAX];	/* extent list */
		xfs_dir2_sf_t	di_dir2sf;	/* shortform directory v2 */
		char		di_c[1];	/* local contents */
		uint32_t	di_dev;		/* device for S_IFCHR/S_IFBLK */
		uint8_t		di_muuid[16];	/* mount point value */
		char		di_symlink[1];	/* local symbolic link */
	} di_u;
	union {
		xfs_bmdr_block_t di_abmbt;	/* btree root block */
		xfs_bmbt_rec_32_t di_abmx[XFS_EXTENT_MAX];	/* extent list */
		xfs_attr_shortform_t di_attrsf;	/* shortform attribute list */
	} di_a;
} xfs_dinode;
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

    extern TSK_RETVAL_ENUM
        xfs_dir_open_meta(TSK_FS_INFO * a_fs, TSK_FS_DIR ** a_fs_dir,
        TSK_INUM_T a_addr);

#ifdef __cplusplus
}
#endif
#endif