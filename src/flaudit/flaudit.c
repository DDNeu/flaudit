/*
 *flaudit - consumes Lustre Changelogs using liblustreapi and write output in json. Based on https://github.com/stanford-rc/lauditd
 *
 *Copyright (C) 2022 DDN
 *
 *This program is free software; you can redistribute it and/or modify
 *it under the terms of the GNU General Public License as published by
 *the Free Software Foundation; either version 3 of the License, or
 *(at your option) any later version.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *
 *You should have received a copy of the GNU General Public License
 *along with this program; if not, write to the Free Software Foundation,
 *Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#
#ifdef HAVE_CONFIG_H
#include "flaudit_config.h"

# endif
#include <errno.h>
#include <assert.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <search.h>
#include <lustre/lustreapi.h>

//COLAVINCENZO to resolve UID/GID
#include <pwd.h>
#include <grp.h>

# ifndef LPX64
#define LPX64 "%#llx"#
#endif

# ifndef PATH_MAX
#define PATH_MAX 1024
# endif

#define IGNORE_TYPES_H_MAX 30 /* > max number of changelogs types */

enum flaudit_enqueue_status
{
    FLAUDIT_ENQUEUE_SUCCESS = 0, /* read/write success and more to process */
    FLAUDIT_ENQUEUE_READER_FAILURE = 1, /* changelog reader failure (or EOF) */
};

static int TerminateSig;

static void usage(void)
{
    fprintf(stderr, "Usage: flaudit[-u cl1][-b batch_size] <mdtname>\n");
}

static void flaudit_sigterm(int signo)
{
    TerminateSig = 1;
}

// COLAVINCENZO fid2path
static void fid2path(const char *fsname, struct lu_fid *lu_fid, __u64 *rectmp, char *path)
{
    char fid[32];
    int lnktmp = 0;
    sprintf(fid, DFID, PFID(lu_fid));
    llapi_fid2path(fsname, fid, path, PATH_MAX, rectmp, &lnktmp);
}

// COLAVINCENZO resolve UID/GID
static void res_uidgid(const int uid, const int gid, char *user, char *group)
{
    struct passwd *pwd;
    struct group *grp;

    if ((pwd = getpwuid(uid)) != NULL)
        sprintf(user, "%s", pwd->pw_name);
    else
        sprintf(user, "%s", uid);

    if ((grp = getgrgid(gid)) != NULL)
        sprintf(group, "%s", grp->gr_name);
    else
        sprintf(group, "%s", gid);
}

static int flaudit_writerec(const char *device, struct changelog_rec *rec)
{
    int rc;
    time_t secs;
    struct tm ts;
    char buf[32];
    char *linebufptr;
    int linebuflen;
    char linebuf[8192];
    // COLAVINCENZO fixed size buffer for fid2path
    char path[PATH_MAX];
    __u64 recno;
    // COLAVINCENZO extract fsname from device, search for a lustreapi call
    char *fsname = strtok(strdup(device), "-");
    char user[32];
    char group[32];

    linebufptr = linebuf;
    linebuflen = sizeof(linebuf);

    secs = rec->cr_time >> 30;
    localtime_r(&secs, &ts);

    strftime(buf, sizeof(buf), "%s", &ts);
    rc = snprintf(linebufptr, linebuflen, "[%s.%06d,{\"mdt\":\"%s\",\"id\":\"%llu\",\"type\":\"%-5s\",\"flags\":\"0x%x\"",
        buf, (int)(rec->cr_time &((1 << 30) - 1)),
        device, rec->cr_index, changelog_type2str(rec->cr_type),
        rec->cr_flags &CLF_FLAGMASK);
    if (rc < 0 || rc >= linebuflen)
        goto error;

    linebufptr += rc;
    linebuflen -= rc;

    if (rec->cr_flags &CLF_EXTRA_FLAGS)
    {
        struct changelog_ext_uidgid *uidgid = changelog_rec_uidgid(rec);
        // COLAVINCENZO resolve UID/GID
        res_uidgid(uidgid->cr_uid, uidgid->cr_gid, user, group);
        rc = snprintf(linebufptr, linebuflen, ",\"uid\":\"%llu\",\"gid\":\"%llu\",\"user\":\"%s\",\"group\":\"%s\"",
            uidgid->cr_uid, uidgid->cr_gid, user, group);
        if (rc < 0 || rc >= linebuflen)
            goto error;
        linebufptr += rc;
        linebuflen -= rc;
    }

    if (rec->cr_flags &CLF_JOBID)
    { const char *jobid = (const char *) changelog_rec_jobid(rec);
        if (*jobid)
        {
            rc = snprintf(linebufptr, linebuflen, ",\"jobid\":\"%s\"", jobid);
            if (rc < 0 || rc >= linebuflen)
                goto error;
            linebufptr += rc;
            linebuflen -= rc;
        }
    }

    // COLAVINCENZO "fix" NIDs never show up
    //    if (rec->cr_flags &CLFE_NID) {
    if (rec->cr_flags)
    {
        struct changelog_ext_nid *nid = changelog_rec_nid(rec);
        libcfs_nid2str_r(nid->cr_nid, buf);
        rc = snprintf(linebufptr, linebuflen, ",\"nid\":\"%s\"", buf);
        if (rc < 0 || rc >= linebuflen)
            goto error;
        linebufptr += rc;
        linebuflen -= rc;
    }
    //strcpy(path,changelog_rec_name(rec));
    rc = snprintf(linebufptr, linebuflen, ",\"target\":\""DFID"\",\"target_name\":\"%.*s\"", PFID(&rec->cr_tfid), changelog_rec_name(rec));

    if (rc < 0 || rc >= linebuflen)
        goto error;
    linebufptr += rc;
    linebuflen -= rc;

    if (rec->cr_flags &CLF_RENAME)
    {
        struct changelog_ext_rename *rnm;
        rnm = changelog_rec_rename(rec);
        // COLAVINCENZO added source_parent_name via fid2path
        if (!fid_is_zero(&rnm->cr_sfid))
        {
            recno = -1;
            fid2path(fsname, &rnm->cr_spfid, &recno, path);
            rc = snprintf(linebufptr, linebuflen,
                ",\"source\":\""
                DFID "\",\"source_parent\":\""
                DFID "\",\"source_name\":\"%.*s\",\"source_parent_name\":\"%s\"",
                PFID(&rnm->cr_sfid), PFID(&rnm->cr_spfid), changelog_rec_snamelen(rec), changelog_rec_sname(rec), path);
            if (rc < 0 || rc >= linebuflen)
                goto error;
            linebufptr += rc;
            linebuflen -= rc;
        }
    }

    if (rec->cr_namelen)
    {
        // COLAVINCENZO added parent_name via fid2path, change label "name" to "target_name"
        fid2path(fsname, &rec->cr_pfid, &rec->cr_index, path);
        rc = snprintf(linebufptr, linebuflen, ",\"parent\":\""
            DFID "\",\"target_name\":\"%.*s\",\"parent_name\":\"%s\"",
            PFID(&rec->cr_pfid), rec->cr_namelen, changelog_rec_name(rec), path);
        if (rc < 0 || rc >= linebuflen)
            goto error;
        linebufptr += rc;
        linebuflen -= rc;
    }
    // COLAVINCENZO added \r for stdout
    rc = snprintf(linebufptr, linebuflen, "}]\n\r");
    if (rc < 0 || rc >= linebuflen)
        goto error;

    rc = fprintf(stdout, "%s", linebuf);
    // COLAVINCENZO fflush for stdout
    fflush(stdout);
    // COLAVINCENZO cleanup fsname
    free(fsname);
    return (rc < 0) ? 1 : 0;
    error:
        fprintf(stderr, "FATAL: line buffer overflow (%s)\n", strerror(errno));
    exit(EXIT_FAILURE);
}

static int flaudit_enqueue(const char *device, int batch_size, long long *recpos)
{
    int flags = CHANGELOG_FLAG_JOBID |
        CHANGELOG_FLAG_EXTRA_FLAGS;
    int batch_count = 0;
    void *ctx;
    struct changelog_rec *rec;
    int rc;
    int status = FLAUDIT_ENQUEUE_READER_FAILURE;
    ENTRY e;

    rc = llapi_changelog_start(&ctx, flags, device, *recpos + 1);
    if (rc < 0)
    {
        fprintf(stderr, "llapi_changelog_start device=%s recid=%lld rc=%d (%s)\n",
            device, *recpos + 1, rc, strerror(errno));
        goto exit_enqueue;
    }

    rc = llapi_changelog_set_xflags(ctx, CHANGELOG_EXTRA_FLAG_UIDGID |
        CHANGELOG_EXTRA_FLAG_NID |
        CHANGELOG_EXTRA_FLAG_OMODE);

    if (rc < 0)
    {
        fprintf(stderr, "llapi_changelog_set_xflags rc=%d\n", rc);
        goto exit_enqueue;
    }

    while ((rc = llapi_changelog_recv(ctx, &rec)) == 0)
    {
        e.key = (char *) changelog_type2str(rec->cr_type);

        flaudit_writerec(device, rec);

        *recpos = rec->cr_index;

        rc = llapi_changelog_free(&rec);
        if (rc < 0)
        {
            fprintf(stderr, "llapi_changelog_free rc=%d\n", rc);
            break;
        }

        if (++batch_count >= batch_size)
            break;
    }

    if (rc == 0)
    {
        status = FLAUDIT_ENQUEUE_SUCCESS;
    }

    rc = llapi_changelog_fini(&ctx);
    if (rc)
    {
        fprintf(stderr, "llapi_changelog_fini rc=%d\n", rc);
        status = FLAUDIT_ENQUEUE_READER_FAILURE;
    }

    exit_enqueue:
        return status;
}

int main(int ac, char ** av)
{
    struct sigaction ignore_action;
    struct sigaction term_action; const char *mdtname = NULL;
    int c;
    int rc;
    int batch_size = 1;
    char clid[64] = { 0 };
    char *p;
    struct stat statbuf;
    long long recpos = 0LL;

    if (ac < 2)
    {
        usage();
        return 1;
    }

    if (!hcreate(IGNORE_TYPES_H_MAX))
    {
        fprintf(stderr, "hcreate() failed: %s\n", strerror(errno));
        return 1;
    }

    while ((c = getopt(ac, av, "b:u:")) != -1)
    {
        switch (c)
        {
            case 'b':
                batch_size = atoi(optarg);
                break;
            case 'u':
                strncpy(clid, optarg, sizeof(clid));
                clid[sizeof(clid) - 1] = '\0';
                break;
            case '?':
                usage();
                return 1;
        }
    }

    if (strlen(clid) == 0)
    {
        fprintf(stderr, "Missing changelog registration id (-u)\n");
        return 1;
    }

    ac -= optind;
    av += optind;

    if (ac < 1)
    {
        usage();
        return 2;
    }

    mdtname = av[0];

    /* Ignore SIGPIPEs -- can occur if the reader goes away. */
    memset(&ignore_action, 0, sizeof(ignore_action));
    ignore_action.sa_handler = SIG_IGN;
    sigemptyset(&ignore_action.sa_mask);
    rc = sigaction(SIGPIPE, &ignore_action, NULL);
    if (rc)
    {
        rc = -errno;
        fprintf(stderr, "cannot setup signal handler (%d): %s\n",
            SIGPIPE, strerror(-rc));
        return 1;
    }

    /* SIGTERM/SIGINT: clean named pipe on normal exit */
    memset(&term_action, 0, sizeof(term_action));
    term_action.sa_handler = flaudit_sigterm;
    sigemptyset(&term_action.sa_mask);
    rc = sigaction(SIGTERM, &term_action, NULL);
    if (rc)
    {
        rc = -errno;
        fprintf(stderr, "cannot setup signal handler (%d): %s\n",
            SIGTERM, strerror(-rc));
        return 1;
    }
    rc = sigaction(SIGINT, &term_action, NULL);
    if (rc)
    {
        rc = -errno;
        fprintf(stderr, "cannot setup signal handler (%d): %s\n",
            SIGINT, strerror(-rc));
        return 1;
    }

    while (1)
    {
        long long startrec = recpos;

        switch (flaudit_enqueue(mdtname, batch_size, &recpos))
        {
            case FLAUDIT_ENQUEUE_READER_FAILURE:
                /* EOF or error from changelogs */
                sleep(1);
            case FLAUDIT_ENQUEUE_SUCCESS:
                break;
        }

        if (recpos > startrec)
        {
            // clear changelogs read
            rc = llapi_changelog_clear(mdtname, clid, recpos);
            if (rc < 0)
            {
                fprintf(stderr, "llapi_changelog_clear recpos=%lld rc=%d\n",
                    recpos, rc);
            }
        }

        if (TerminateSig)
        {
            break;
        }
    }
	
    return 0;
}
