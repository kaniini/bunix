#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

enum {
	STATID_STATX_BASIC_STATS = 0x7ff,
	STATID_AT_FDCWD = -100,
};

static unsigned long long load_u64(const unsigned char *data, unsigned int off)
{
	unsigned long long value = 0;

	for (unsigned int i = 0; i < 8; i++) {
		value |= ((unsigned long long)data[off + i]) << (i * 8);
	}
	return value;
}

static int same_identity(const struct stat *a, const struct stat *b)
{
	return a->st_dev == b->st_dev && a->st_ino == b->st_ino;
}

static int check_stat_fstat(const char *path)
{
	struct stat st;
	struct stat fst;
	int fd;

	if (stat(path, &st) != 0) {
		perror("statid stat");
		return -1;
	}
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("statid open");
		return -1;
	}
	if (fstat(fd, &fst) != 0) {
		perror("statid fstat");
		close(fd);
		return -1;
	}
	close(fd);
	if (!same_identity(&st, &fst) || st.st_size != fst.st_size ||
	    st.st_mode != fst.st_mode) {
		printf("statid stat/fstat mismatch path=%s stat=(%llu,%llu,%llu) fstat=(%llu,%llu,%llu)\n",
		       path, (unsigned long long)st.st_dev,
		       (unsigned long long)st.st_ino,
		       (unsigned long long)st.st_size,
		       (unsigned long long)fst.st_dev,
		       (unsigned long long)fst.st_ino,
		       (unsigned long long)fst.st_size);
		return -1;
	}
	return 0;
}

static int check_distinct(const char *left, const char *right)
{
	struct stat a;
	struct stat b;

	if (stat(left, &a) != 0 || stat(right, &b) != 0) {
		perror("statid distinct stat");
		return -1;
	}
	if (same_identity(&a, &b)) {
		printf("statid distinct collapse left=%s right=%s dev=%llu ino=%llu\n",
		       left, right, (unsigned long long)a.st_dev,
		       (unsigned long long)a.st_ino);
		return -1;
	}
	return 0;
}

static int check_hardlink(void)
{
	struct stat a;
	struct stat b;
	int fd;

	(void)unlink("/tmp/statid-a");
	(void)unlink("/tmp/statid-b");
	fd = open("/tmp/statid-a", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0 || write(fd, "statid\n", 7) != 7) {
		perror("statid create");
		if (fd >= 0) {
			close(fd);
		}
		return -1;
	}
	close(fd);
	if (link("/tmp/statid-a", "/tmp/statid-b") != 0 ||
	    stat("/tmp/statid-a", &a) != 0 ||
	    stat("/tmp/statid-b", &b) != 0) {
		perror("statid hardlink");
		return -1;
	}
	if (!same_identity(&a, &b) || a.st_nlink < 2 || b.st_nlink < 2) {
		printf("statid hardlink identity dev=%llu/%llu ino=%llu/%llu nlink=%llu/%llu\n",
		       (unsigned long long)a.st_dev,
		       (unsigned long long)b.st_dev,
		       (unsigned long long)a.st_ino,
		       (unsigned long long)b.st_ino,
		       (unsigned long long)a.st_nlink,
		       (unsigned long long)b.st_nlink);
		return -1;
	}
	return 0;
}

static int check_symlink(void)
{
	struct stat target;
	struct stat follow;
	struct stat linkst;

	(void)unlink("/tmp/statid-link");
	if (symlink("/hello.txt", "/tmp/statid-link") != 0 ||
	    stat("/hello.txt", &target) != 0 ||
	    stat("/tmp/statid-link", &follow) != 0 ||
	    lstat("/tmp/statid-link", &linkst) != 0) {
		perror("statid symlink");
		return -1;
	}
	if (!same_identity(&target, &follow) ||
	    (linkst.st_mode & S_IFMT) != S_IFLNK ||
	    same_identity(&target, &linkst)) {
		printf("statid symlink target=(%llu,%llu) follow=(%llu,%llu) link=(%llu,%llu,%o)\n",
		       (unsigned long long)target.st_dev,
		       (unsigned long long)target.st_ino,
		       (unsigned long long)follow.st_dev,
		       (unsigned long long)follow.st_ino,
		       (unsigned long long)linkst.st_dev,
		       (unsigned long long)linkst.st_ino,
		       (unsigned int)linkst.st_mode);
		return -1;
	}
	return 0;
}

static int check_unionfs(void)
{
	struct stat lower;
	struct stat upper;
	struct stat hard;
	int fd;

	(void)unlink("/statid-upper");
	(void)unlink("/statid-lower-hard");

	fd = open("/statid-upper", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0 || write(fd, "upper\n", 6) != 6) {
		perror("statid union create");
		if (fd >= 0) {
			close(fd);
		}
		return -1;
	}
	close(fd);

	if (check_stat_fstat("/statid-upper") != 0 ||
	    stat("/hello.txt", &lower) != 0 ||
	    stat("/statid-upper", &upper) != 0) {
		perror("statid union stat");
		return -1;
	}
	if (same_identity(&lower, &upper)) {
		printf("statid union lower/upper collapse dev=%llu ino=%llu\n",
		       (unsigned long long)lower.st_dev,
		       (unsigned long long)lower.st_ino);
		return -1;
	}

	if (link("/hello.txt", "/statid-lower-hard") != 0 ||
	    stat("/hello.txt", &lower) != 0 ||
	    stat("/statid-lower-hard", &hard) != 0) {
		perror("statid union hardlink");
		return -1;
	}
	if (!same_identity(&lower, &hard) ||
	    lower.st_nlink < 2 || hard.st_nlink < 2) {
		printf("statid union hardlink identity dev=%llu/%llu ino=%llu/%llu nlink=%llu/%llu\n",
		       (unsigned long long)lower.st_dev,
		       (unsigned long long)hard.st_dev,
		       (unsigned long long)lower.st_ino,
		       (unsigned long long)hard.st_ino,
		       (unsigned long long)lower.st_nlink,
		       (unsigned long long)hard.st_nlink);
		return -1;
	}
	return 0;
}

static int check_statx(const char *path)
{
	struct stat st;
	unsigned char statx[256];
	long rc;
	unsigned long long ino;
	unsigned long long size;

	memset(statx, 0, sizeof(statx));
	if (stat(path, &st) != 0) {
		perror("statid statx stat");
		return -1;
	}
	rc = syscall(SYS_statx, STATID_AT_FDCWD, path, 0,
		     STATID_STATX_BASIC_STATS,
		     statx);
	if (rc != 0) {
		perror("statid statx");
		return -1;
	}
	ino = load_u64(statx, 32);
	size = load_u64(statx, 40);
	if (ino != (unsigned long long)st.st_ino ||
	    size != (unsigned long long)st.st_size) {
		printf("statid statx mismatch path=%s stat=(%llu,%llu) statx=(%llu,%llu)\n",
		       path, (unsigned long long)st.st_ino,
		       (unsigned long long)st.st_size, ino, size);
		return -1;
	}
	return 0;
}

static int path_exists(const char *path)
{
	return access(path, F_OK) == 0;
}

static int check_synthetic_rootfs(void)
{
	if (!path_exists("/hello.txt")) {
		return 0;
	}
	if (check_stat_fstat("/hello.txt") != 0 ||
	    check_stat_fstat("/bin/musl-hello") != 0 ||
	    check_distinct("/bin/musl-hello", "/bin/dyn-hello") != 0 ||
	    check_hardlink() != 0 ||
	    check_symlink() != 0 ||
	    check_unionfs() != 0 ||
	    check_statx("/hello.txt") != 0) {
		return -1;
	}
	return 0;
}

static int check_openrc_dsos(void)
{
	if (!path_exists("/usr/lib/librc.so.1") &&
	    !path_exists("/lib/librc.so.1")) {
		return 0;
	}

	if (check_stat_fstat("/usr/lib/librc.so.1") != 0 ||
	    check_stat_fstat("/usr/lib/libeinfo.so.1") != 0 ||
	    check_distinct("/usr/lib/librc.so.1",
			   "/usr/lib/libeinfo.so.1") != 0 ||
	    check_statx("/usr/lib/librc.so.1") != 0) {
		return -1;
	}

	puts("linux statid dso ok");
	return 0;
}

int main(void)
{
	if (check_synthetic_rootfs() != 0 ||
	    check_openrc_dsos() != 0) {
		return 1;
	}
	puts("linux statid ok");
	return 0;
}
