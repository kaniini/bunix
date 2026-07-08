static long rv_syscall0(long number)
{
	register long a0 __asm__("a0") = 0;
	register long a7 __asm__("a7") = number;

	__asm__ volatile("ecall" : "+r"(a0) : "r"(a7) : "memory");
	return a0;
}

static long rv_syscall3(long number, long arg0, long arg1, long arg2)
{
	register long a0 __asm__("a0") = arg0;
	register long a1 __asm__("a1") = arg1;
	register long a2 __asm__("a2") = arg2;
	register long a7 __asm__("a7") = number;

	__asm__ volatile("ecall"
			 : "+r"(a0)
			 : "r"(a1), "r"(a2), "r"(a7)
			 : "memory");
	return a0;
}

static unsigned long str_len(const char *text)
{
	unsigned long len = 0;

	while (text[len] != '\0') {
		len++;
	}
	return len;
}

static void write_text(const char *text)
{
	char buffer[96];
	unsigned long len = str_len(text);

	if (len > sizeof(buffer)) {
		len = sizeof(buffer);
	}
	for (unsigned long i = 0; i < len; i++) {
		buffer[i] = text[i];
	}
	(void)rv_syscall3(64, 1, (long)buffer, (long)len);
}

static void append_text(char *out, unsigned long *cursor, const char *text)
{
	for (unsigned long i = 0; text[i] != '\0'; i++) {
		out[(*cursor)++] = text[i];
	}
}

static void append_ulong(char *out, unsigned long *cursor, unsigned long value)
{
	char digits[32];
	unsigned long count = 0;

	if (value == 0) {
		out[(*cursor)++] = '0';
		return;
	}
	while (value != 0 && count < sizeof(digits)) {
		digits[count++] = (char)('0' + (value % 10));
		value /= 10;
	}
	while (count != 0) {
		out[(*cursor)++] = digits[--count];
	}
}

int main(void)
{
	const long pid = rv_syscall0(172);
	const long ppid = rv_syscall0(173);
	const long uid = rv_syscall0(174);
	const long euid = rv_syscall0(175);
	const long gid = rv_syscall0(176);
	const long egid = rv_syscall0(177);
	const long tid = rv_syscall0(178);
	char line[160];
	unsigned long cursor = 0;

	append_text(line, &cursor, "rv64 syscall smoke pid=");
	append_ulong(line, &cursor, (unsigned long)pid);
	append_text(line, &cursor, " ppid=");
	append_ulong(line, &cursor, (unsigned long)ppid);
	append_text(line, &cursor, " tid=");
	append_ulong(line, &cursor, (unsigned long)tid);
	append_text(line, &cursor, " uid=");
	append_ulong(line, &cursor, (unsigned long)uid);
	append_text(line, &cursor, " gid=");
	append_ulong(line, &cursor, (unsigned long)gid);
	append_text(line, &cursor, " euid=");
	append_ulong(line, &cursor, (unsigned long)euid);
	append_text(line, &cursor, " egid=");
	append_ulong(line, &cursor, (unsigned long)egid);
	line[cursor++] = '\n';
	(void)rv_syscall3(64, 1, (long)line, (long)cursor);

	if (pid <= 0 || tid <= 0 || uid != 0 || gid != 0 ||
	    euid != 0 || egid != 0) {
		write_text("rv64 syscall smoke failed\n");
		return 1;
	}
	write_text("rv64 syscall smoke ok\n");
	return 0;
}
