// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2000-2005 Silicon Graphics, Inc.
 * All Rights Reserved.
 */
#ifndef __XFS_QUOTA_H__
#define __XFS_QUOTA_H__

#include "xfs_quota_defs.h"

/*
 * Kernel only quota definitions and functions
 */

struct xfs_trans;
struct xfs_buf;

/*
 * This check is done typically without holding the inode lock;
 * that may seem racy, but it is harmless in the context that it is used.
 * The inode cannot go inactive as long a reference is kept, and
 * therefore if dquot(s) were attached, they'll stay consistent.
 * If, for example, the ownership of the inode changes while
 * we didn't have the inode locked, the appropriate dquot(s) will be
 * attached atomically.
 */
#define XFS_NOT_DQATTACHED(mp, ip) \
	((XFS_IS_UQUOTA_ON(mp) && (ip)->i_udquot == NULL) || \
	 (XFS_IS_GQUOTA_ON(mp) && (ip)->i_gdquot == NULL) || \
	 (XFS_IS_PQUOTA_ON(mp) && (ip)->i_pdquot == NULL))

#define XFS_QM_NEED_QUOTACHECK(mp) \
	((XFS_IS_UQUOTA_ON(mp) && \
		(mp->m_sb.sb_qflags & XFS_UQUOTA_CHKD) == 0) || \
	 (XFS_IS_GQUOTA_ON(mp) && \
		(mp->m_sb.sb_qflags & XFS_GQUOTA_CHKD) == 0) || \
	 (XFS_IS_PQUOTA_ON(mp) && \
		(mp->m_sb.sb_qflags & XFS_PQUOTA_CHKD) == 0))

static inline uint
xfs_quota_chkd_flag(
	xfs_dqtype_t		type)
{
	switch (type) {
	case XFS_DQTYPE_USER:
		return XFS_UQUOTA_CHKD;
	case XFS_DQTYPE_GROUP:
		return XFS_GQUOTA_CHKD;
	case XFS_DQTYPE_PROJ:
		return XFS_PQUOTA_CHKD;
	default:
		return 0;
	}
}

/*
 * The structure kept inside the xfs_trans_t keep track of dquot changes
 * within a transaction and apply them later.
 */
struct xfs_dqtrx {
	struct xfs_dquot *qt_dquot;	  /* the dquot this refers to */

	uint64_t	qt_blk_res;	  /* blks reserved on a dquot */
	int64_t		qt_bcount_delta;  /* dquot blk count changes */
	int64_t		qt_delbcnt_delta; /* delayed dquot blk count changes */

	uint64_t	qt_rtblk_res;	  /* # blks reserved on a dquot */
	uint64_t	qt_rtblk_res_used;/* # blks used from reservation */
	int64_t		qt_rtbcount_delta;/* dquot realtime blk changes */
	int64_t		qt_delrtb_delta;  /* delayed RT blk count changes */

	uint64_t	qt_ino_res;	  /* inode reserved on a dquot */
	uint64_t	qt_ino_res_used;  /* inodes used from the reservation */
	int64_t		qt_icount_delta;  /* dquot inode count changes */
};

enum xfs_apply_dqtrx_type {
	XFS_APPLY_DQTRX_COMMIT = 0,
	XFS_APPLY_DQTRX_UNRESERVE,
};

/*
 * Parameters for applying dqtrx changes to a dquot.  The hook function arg
 * parameter is enum xfs_apply_dqtrx_type.
 */
struct xfs_apply_dqtrx_params {
	uintptr_t		tx_id;
	xfs_ino_t		ino;
	xfs_dqtype_t		q_type;
	xfs_dqid_t		q_id;
};

#ifdef CONFIG_XFS_QUOTA
extern void xfs_trans_dup_dqinfo(struct xfs_trans *, struct xfs_trans *);
extern void xfs_trans_free_dqinfo(struct xfs_trans *);
extern void xfs_trans_mod_dquot_byino(struct xfs_trans *, struct xfs_inode *,
		uint, int64_t);
extern void xfs_trans_apply_dquot_deltas(struct xfs_trans *);
void xfs_trans_unreserve_and_mod_dquots(struct xfs_trans *tp,
		bool already_locked);
int xfs_trans_reserve_quota_nblks(struct xfs_trans *tp, struct xfs_inode *ip,
		int64_t dblocks, int64_t rblocks, bool force);
extern int xfs_trans_reserve_quota_bydquots(struct xfs_trans *,
		struct xfs_mount *, struct xfs_dquot *,
		struct xfs_dquot *, struct xfs_dquot *, int64_t, long, uint);
int xfs_trans_reserve_quota_icreate(struct xfs_trans *tp,
		struct xfs_dquot *udqp, struct xfs_dquot *gdqp,
		struct xfs_dquot *pdqp, int64_t dblocks);

extern int xfs_qm_vop_dqalloc(struct xfs_inode *, kuid_t, kgid_t,
		prid_t, uint, struct xfs_dquot **, struct xfs_dquot **,
		struct xfs_dquot **);
extern void xfs_qm_vop_create_dqattach(struct xfs_trans *, struct xfs_inode *,
		struct xfs_dquot *, struct xfs_dquot *, struct xfs_dquot *);
extern int xfs_qm_vop_rename_dqattach(struct xfs_inode **);
extern struct xfs_dquot *xfs_qm_vop_chown(struct xfs_trans *,
		struct xfs_inode *, struct xfs_dquot **, struct xfs_dquot *);
extern int xfs_qm_dqattach(struct xfs_inode *);
extern int xfs_qm_dqattach_locked(struct xfs_inode *ip, bool doalloc);
extern void xfs_qm_dqdetach(struct xfs_inode *);
extern void xfs_qm_dqrele(struct xfs_dquot *);
extern void xfs_qm_statvfs(struct xfs_inode *, struct kstatfs *);
extern int xfs_qm_newmount(struct xfs_mount *, uint *, uint *);
void xfs_qm_resume_quotaon(struct xfs_mount *mp);
extern void xfs_qm_mount_quotas(struct xfs_mount *);
extern void xfs_qm_unmount(struct xfs_mount *);
extern void xfs_qm_unmount_quotas(struct xfs_mount *);
bool xfs_inode_near_dquot_enforcement(struct xfs_inode *ip, xfs_dqtype_t type);
int xfs_quota_reserve_blkres(struct xfs_inode *ip, int64_t blocks);

# ifdef CONFIG_XFS_LIVE_HOOKS
void xfs_trans_mod_ino_dquot(struct xfs_trans *tp, struct xfs_inode *ip,
		struct xfs_dquot *dqp, unsigned int field, int64_t delta);

struct xfs_quotainfo;

struct xfs_dqtrx_hook {
	struct xfs_hook		mod_hook;
	struct xfs_hook		apply_hook;
};

void xfs_dqtrx_hook_disable(void);
void xfs_dqtrx_hook_enable(void);

int xfs_dqtrx_hook_add(struct xfs_quotainfo *qi, struct xfs_dqtrx_hook *hook);
void xfs_dqtrx_hook_del(struct xfs_quotainfo *qi, struct xfs_dqtrx_hook *hook);
void xfs_dqtrx_hook_setup(struct xfs_dqtrx_hook *hook, notifier_fn_t mod_fn,
		notifier_fn_t apply_fn);
# else
#  define xfs_trans_mod_ino_dquot(tp, ip, dqp, field, delta) \
		xfs_trans_mod_dquot((tp), (dqp), (field), (delta))
# endif /* CONFIG_XFS_LIVE_HOOKS */

#else
static inline int
xfs_qm_vop_dqalloc(struct xfs_inode *ip, kuid_t kuid, kgid_t kgid,
		prid_t prid, uint flags, struct xfs_dquot **udqp,
		struct xfs_dquot **gdqp, struct xfs_dquot **pdqp)
{
	*udqp = NULL;
	*gdqp = NULL;
	*pdqp = NULL;
	return 0;
}
#define xfs_trans_dup_dqinfo(tp, tp2)
#define xfs_trans_free_dqinfo(tp)
static inline void xfs_trans_mod_dquot_byino(struct xfs_trans *tp,
		struct xfs_inode *ip, uint field, int64_t delta)
{
}
#define xfs_trans_apply_dquot_deltas(tp)
#define xfs_trans_unreserve_and_mod_dquots(tp, a)
static inline int xfs_trans_reserve_quota_nblks(struct xfs_trans *tp,
		struct xfs_inode *ip, int64_t dblocks, int64_t rblocks,
		bool force)
{
	return 0;
}
static inline int xfs_trans_reserve_quota_bydquots(struct xfs_trans *tp,
		struct xfs_mount *mp, struct xfs_dquot *udqp,
		struct xfs_dquot *gdqp, struct xfs_dquot *pdqp,
		int64_t nblks, long nions, uint flags)
{
	return 0;
}

static inline int
xfs_trans_reserve_quota_icreate(struct xfs_trans *tp, struct xfs_dquot *udqp,
		struct xfs_dquot *gdqp, struct xfs_dquot *pdqp, int64_t dblocks)
{
	return 0;
}

#define xfs_qm_vop_create_dqattach(tp, ip, u, g, p)
#define xfs_qm_vop_rename_dqattach(it)					(0)
#define xfs_qm_vop_chown(tp, ip, old, new)				(NULL)
#define xfs_qm_dqattach(ip)						(0)
#define xfs_qm_dqattach_locked(ip, fl)					(0)
#define xfs_qm_dqdetach(ip)
#define xfs_qm_dqrele(d)			do { (d) = (d); } while(0)
#define xfs_qm_statvfs(ip, s)			do { } while(0)
#define xfs_qm_newmount(mp, a, b)					(0)
#define xfs_qm_resume_quotaon(mp)		((void)0)
#define xfs_qm_mount_quotas(mp)
#define xfs_qm_unmount(mp)
#define xfs_qm_unmount_quotas(mp)
#define xfs_inode_near_dquot_enforcement(ip, type)			(false)

static inline int xfs_quota_reserve_blkres(struct xfs_inode *ip, int64_t blocks)
{
	return 0;
}

# ifdef CONFIG_XFS_LIVE_HOOKS
#  define xfs_dqtrx_hook_enable()		((void)0)
#  define xfs_dqtrx_hook_disable()		((void)0)
# endif /* CONFIG_XFS_LIVE_HOOKS */

#endif /* CONFIG_XFS_QUOTA */

static inline void
xfs_quota_unreserve_blkres(struct xfs_inode *ip, uint64_t blocks)
{
	/* don't return an error as unreserving quotas can't fail */
	xfs_quota_reserve_blkres(ip, -(int64_t)blocks);
}

extern int xfs_mount_reset_sbqflags(struct xfs_mount *);

#endif	/* __XFS_QUOTA_H__ */
