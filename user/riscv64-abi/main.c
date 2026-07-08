#include <bunix/syscall.h>

int main(int argc, char **argv, char **envp)
{
	(void)argv;
	(void)envp;
	return (int)bunix_syscall1(BUNIX_SYSCALL_EXIT, (u64)argc);
}
