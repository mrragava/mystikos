// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <libos/ramfs.h>
#include <libos/mount.h>
#include <libos/file.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "run_t.h"

const char alpha[] = "abcdefghijklmnopqrstuvwxyz";
const char ALPHA[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void test_misc()
{
    libos_fs_t* fs;
    libos_file_t* file = NULL;

    if (libos_init_ramfs(&fs) != 0)
    {
        fprintf(stderr, "libos_init_ramfs() failed\n");
        abort();
    }

    assert(fs != NULL);

    /* Open the root directory */
    {
        if ((*fs->fs_open)(fs, "/", O_RDONLY, 0, &file) != 0)
        {
            fprintf(stderr, "fs_open() failed\n");
            abort();
        }

        assert(file != NULL);
    }

    /* Read the directory entries in the root directory */
    {
        size_t i = 0;
        ssize_t n;
        struct dirent buf;

        while ((n = (*fs->fs_read)(fs, file, &buf, sizeof(buf))) > 0)
        {
            assert(n == sizeof(buf));

            if (i == 0)
            {
                assert(strcmp(buf.d_name, ".") == 0);
            }
            else if (i == 1)
            {
                assert(strcmp(buf.d_name, "..") == 0);
            }

            i++;
        }
    }

    /* Close the root directory */
    {
        if ((*fs->fs_close)(fs, file) != 0)
        {
            fprintf(stderr, "fs_close() failed\n");
            abort();
        }

        file = NULL;
    }

    /* Open a new file */
    {
        int flags = O_WRONLY | O_CREAT;

        if ((*fs->fs_open)(fs, "/file", flags, 0, &file) != 0)
        {
            fprintf(stderr, "fs_open() failed\n");
            abort();
        }
    }

    /* Write to the new file */
    {
        if ((*fs->fs_write)(fs, file, alpha, sizeof(alpha)) != sizeof(alpha))
        {
            fprintf(stderr, "fs_write() failed\n");
            abort();
        }

        if ((*fs->fs_write)(fs, file, ALPHA, sizeof(ALPHA)) != sizeof(ALPHA))
        {
            fprintf(stderr, "fs_write() failed\n");
            abort();
        }
    }

    /* Close the file */
    if ((*fs->fs_close)(fs, file) != 0)
    {
        fprintf(stderr, "fs_close() failed\n");
        abort();
    }

    /* Reopen the file */
    if ((*fs->fs_open)(fs, "/file", O_RDONLY, 0, &file) != 0)
    {
        fprintf(stderr, "fs_open() failed\n");
        abort();
    }

    /* Read the file */
    {
        char buf[sizeof(alpha)];

        if ((*fs->fs_read)(fs, file, buf, sizeof(buf)) != sizeof(buf))
        {
            fprintf(stderr, "fs_read() failed\n");
            abort();
        }

        assert(strcmp(buf, alpha) == 0);

        if ((*fs->fs_read)(fs, file, buf, sizeof(buf)) != sizeof(buf))
        {
            fprintf(stderr, "fs_read() failed\n");
            abort();
        }

        assert(strcmp(buf, ALPHA) == 0);
    }

    /* test stat() */
    {
        struct stat buf;

        if ((*fs->fs_fstat)(fs, file, &buf) != 0)
        {
            fprintf(stderr, "fs_stat() failed\n");
            abort();
        }

        assert(buf.st_size = sizeof(alpha) + sizeof(ALPHA));
        assert(buf.st_blksize == 512);
        assert(buf.st_blocks == 1);
    }

    if ((*fs->fs_close)(fs, file) != 0)
    {
        fprintf(stderr, "fs_close() failed\n");
        abort();
    }

    /* Release the file system */
    if ((*fs->fs_release)(fs) != 0)
    {
        fprintf(stderr, "fs_close() failed\n");
        abort();
    }
}

void test_readv(void)
{
    int fd;
    char buf[sizeof(ALPHA)];

    assert((fd = libos_open("/test_readv", O_CREAT | O_WRONLY, 0)) >= 0);
    assert(libos_write(fd, alpha, sizeof(alpha)) == sizeof(alpha));
    assert(libos_lseek(fd, 0, SEEK_CUR) == sizeof(alpha));
    assert(libos_write(fd, ALPHA, sizeof(ALPHA)) == sizeof(ALPHA));
    assert(libos_lseek(fd, 0, SEEK_CUR) == sizeof(alpha) + sizeof(ALPHA));
    assert(libos_close(fd) == 0);

    assert((fd = libos_open("/test_readv", O_RDONLY, 0)) >= 0);
    assert(libos_lseek(fd, 0, SEEK_CUR) == 0);
    assert(libos_read(fd, buf, sizeof(buf)) == sizeof(buf));
    assert(libos_lseek(fd, 0, SEEK_CUR) == sizeof(alpha));
    assert(strcmp(buf, alpha) == 0);

    /* Test readv() */
    {
        struct iovec iov[2];
        uint8_t buf1[20];
        uint8_t buf2[7];
        iov[0].iov_base = buf1;
        iov[0].iov_len = sizeof(buf1);
        iov[1].iov_base = buf2;
        iov[1].iov_len = sizeof(buf2);
        assert(libos_readv(fd, iov, 2) == sizeof(ALPHA));
        assert(memcmp(buf1, ALPHA, sizeof(buf1)) == 0);
        assert(memcmp(buf2, ALPHA + sizeof(buf1), sizeof(buf2)) == 0);
    }

    /* Test readv() */
    {
        struct iovec iov[2];
        uint8_t buf1[20];
        uint8_t buf2[7];
        iov[0].iov_base = buf1;
        iov[0].iov_len = sizeof(buf1);
        iov[1].iov_base = buf2;
        iov[1].iov_len = sizeof(buf2);
        assert(libos_lseek(fd, 0, SEEK_SET) == 0);
        assert(libos_readv(fd, iov, 2) == sizeof(alpha));
        assert(memcmp(buf1, alpha, sizeof(buf1)) == 0);
        assert(memcmp(buf2, alpha + sizeof(buf1), sizeof(buf2)) == 0);
    }

    assert(libos_lseek(fd, sizeof(alpha), SEEK_SET) == sizeof(alpha));
    assert(libos_lseek(fd, 0, SEEK_CUR) == sizeof(alpha));

    assert(libos_close(fd) == 0);
}

void test_writev(void)
{
    int fd;
    char buf[sizeof(ALPHA)];

    assert((fd = libos_open("/test_writev", O_CREAT | O_WRONLY, 0)) >= 0);

    struct iovec iov[2];
    iov[0].iov_base = (void*)alpha;
    iov[0].iov_len = sizeof(alpha);
    iov[1].iov_base = (void*)ALPHA;
    iov[1].iov_len = sizeof(ALPHA);

    assert(libos_writev(fd, iov, 2) == sizeof(alpha) + sizeof(ALPHA));
    assert(libos_close(fd) == 0);

    assert((fd = libos_open("/test_writev", O_RDONLY, 0)) >= 0);
    assert(libos_read(fd, buf, sizeof(buf)) == sizeof(buf));
    assert(strcmp(buf, alpha) == 0);
    assert(libos_read(fd, buf, sizeof(buf)) == sizeof(buf));
    assert(strcmp(buf, ALPHA) == 0);

    assert(libos_close(fd) == 0);
}

void test_stat()
{
    struct stat buf;

    assert(libos_stat("/test_readv", &buf) == 0);
    assert(buf.st_size == sizeof(alpha) + sizeof(ALPHA));
}

int run_ecall(void)
{
    libos_fs_t* fs;

    test_misc();

    assert(libos_init_ramfs(&fs) == 0);
    assert(libos_mount(fs, "/") == 0);

    test_readv();
    test_writev();
    test_stat();

    assert((*fs->fs_release)(fs) == 0);

    return 0;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    16*4096, /* NumHeapPages */
    4096, /* NumStackPages */
    2);   /* NumTCS */
