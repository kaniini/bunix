.section .text, "ax"
.global _start
.type _start, @function
_start:
	movq $1, %rax
	movq $1, %rdi
	leaq message(%rip), %rsi
	movq $message_end - message, %rdx
	syscall
	cmpq $message_end - message, %rax
	jne fail

	movq $1, %rax
	movq $99, %rdi
	leaq badfd_message(%rip), %rsi
	movq $badfd_message_end - badfd_message, %rdx
	syscall
	cmpq $-9, %rax
	jne fail

	movq $1, %rax
	movq $1, %rdi
	leaq ok_message(%rip), %rsi
	movq $ok_message_end - ok_message, %rdx
	syscall
	cmpq $ok_message_end - ok_message, %rax
	jne fail

	movq $257, %rax
	movq $-100, %rdi
	leaq hello_path(%rip), %rsi
	xorq %rdx, %rdx
	syscall
	cmpq $3, %rax
	jne fail
	movq %rax, %r12

	movq $39, %rax
	syscall
	testq %rax, %rax
	jle fail
1:
	movq %rax, %r15

	movq $186, %rax
	syscall
	cmpq %r15, %rax
	jne fail

	movq $102, %rax
	syscall
	testq %rax, %rax
	jne fail
	movq $104, %rax
	syscall
	testq %rax, %rax
	jne fail
	movq $107, %rax
	syscall
	testq %rax, %rax
	jne fail
	movq $108, %rax
	syscall
	testq %rax, %rax
	jne fail
	movq $115, %rax
	xorq %rdi, %rdi
	xorq %rsi, %rsi
	syscall
	cmpq $2, %rax
	jne fail
	movq $115, %rax
	movq $2, %rdi
	leaq group_buffer(%rip), %rsi
	syscall
	cmpq $2, %rax
	jne fail
	cmpl $0, group_buffer(%rip)
	jne fail
	cmpl $1, group_buffer + 4(%rip)
	jne fail

	movq $1, %rax
	movq $1, %rdi
	leaq credential_message(%rip), %rsi
	movq $credential_message_end - credential_message, %rdx
	syscall
	cmpq $credential_message_end - credential_message, %rax
	jne fail

	movq $12, %rax
	xorq %rdi, %rdi
	syscall
	testq %rax, %rax
	jle fail
	movq %rax, %r13
	leaq 4096(%r13), %r14

	movq $12, %rax
	movq %r14, %rdi
	syscall
	cmpq %r14, %rax
	jne fail
	movq $12, %rax
	movq %r13, %rdi
	syscall
	cmpq %r13, %rax
	jne fail

	movq $1, %rax
	movq $1, %rdi
	leaq brk_message(%rip), %rsi
	movq $brk_message_end - brk_message, %rdx
	syscall
	cmpq $brk_message_end - brk_message, %rax
	jne fail

	movq $9, %rax
	xorq %rdi, %rdi
	movq $8192, %rsi
	movq $3, %rdx
	movq $0x22, %r10
	movq $-1, %r8
	xorq %r9, %r9
	syscall
	testq %rax, %rax
	js fail
	movq %rax, %rbx
	movq $0x6b63656863207061, %rax
	movq %rax, (%rbx)
	cmpq %rax, (%rbx)
	jne fail
	movq $0x65676170206f7774, %rax
	movq %rax, 4096(%rbx)
	cmpq %rax, 4096(%rbx)
	jne fail
	movq $9, %rax
	movq %rbx, %rdi
	movq $4096, %rsi
	movq $3, %rdx
	movq $0x32, %r10
	movq $-1, %r8
	xorq %r9, %r9
	syscall
	cmpq %rbx, %rax
	jne fail
	movq $0x6465786966207061, %rax
	movq %rax, (%rbx)
	cmpq %rax, (%rbx)
	jne fail
	movq $0x65676170206f7774, %rax
	cmpq %rax, 4096(%rbx)
	jne fail

	movq $1, %rax
	movq $1, %rdi
	leaq mmap_message(%rip), %rsi
	movq $mmap_message_end - mmap_message, %rdx
	syscall
	cmpq $mmap_message_end - mmap_message, %rax
	jne fail

	cmpq $1, %r15
	jne 2f

	movq $57, %rax
	syscall
	testq %rax, %rax
	js fail
	jz fork_child

	movq $61, %rax
	movq $-1, %rdi
	leaq wait_status(%rip), %rsi
	xorq %rdx, %rdx
	syscall
	testq %rax, %rax
	jle fail
	cmpl $0, wait_status(%rip)
	jne fail

	movq $1, %rax
	movq $1, %rdi
	leaq wait_message(%rip), %rsi
	movq $wait_message_end - wait_message, %rdx
	syscall
	cmpq $wait_message_end - wait_message, %rax
	jne fail

	jmp 2f

fork_child:
	movq $0x6465786966207061, %rax
	cmpq %rax, (%rbx)
	jne fail
	movq $0x65676170206f7774, %rax
	cmpq %rax, 4096(%rbx)
	jne fail

	movq $102, %rax
	syscall
	testq %rax, %rax
	jne fail
	movq $104, %rax
	syscall
	testq %rax, %rax
	jne fail
	movq $107, %rax
	syscall
	testq %rax, %rax
	jne fail
	movq $108, %rax
	syscall
	testq %rax, %rax
	jne fail
	movq $115, %rax
	xorq %rdi, %rdi
	xorq %rsi, %rsi
	syscall
	cmpq $2, %rax
	jne fail
	movq $115, %rax
	movq $2, %rdi
	leaq group_buffer(%rip), %rsi
	syscall
	cmpq $2, %rax
	jne fail
	cmpl $0, group_buffer(%rip)
	jne fail
	cmpl $1, group_buffer + 4(%rip)
	jne fail

	movq $1, %rax
	movq $1, %rdi
	leaq fork_credential_message(%rip), %rsi
	movq $fork_credential_message_end - fork_credential_message, %rdx
	syscall

	movq $1, %rax
	movq $1, %rdi
	leaq fork_child_message(%rip), %rsi
	movq $fork_child_message_end - fork_child_message, %rdx
	syscall

	movq $59, %rax
	leaq exec_path(%rip), %rdi
	xorq %rsi, %rsi
	xorq %rdx, %rdx
	syscall

	movq $231, %rax
	movq $1, %rdi
	syscall

2:
	movq $11, %rax
	leaq 4096(%rbx), %rdi
	movq $4096, %rsi
	syscall
	testq %rax, %rax
	jne fail

	movq $1, %rax
	movq $1, %rdi
	leaq munmap_message(%rip), %rsi
	movq $munmap_message_end - munmap_message, %rdx
	syscall
	cmpq $munmap_message_end - munmap_message, %rax
	jne fail

	movq $5, %rax
	movq %r12, %rdi
	leaq stat_buffer(%rip), %rsi
	syscall
	testq %rax, %rax
	jne fail
	cmpl $0100444, stat_buffer + 24(%rip)
	jne fail
	cmpl $0, stat_buffer + 28(%rip)
	jne fail
	cmpl $0, stat_buffer + 32(%rip)
	jne fail
	cmpq $15, stat_buffer + 48(%rip)
	jne fail

	movq $262, %rax
	movq $-100, %rdi
	leaq hello_path(%rip), %rsi
	leaq statat_buffer(%rip), %rdx
	syscall
	testq %rax, %rax
	jne fail
	cmpl $0100444, statat_buffer + 24(%rip)
	jne fail
	cmpl $0, statat_buffer + 28(%rip)
	jne fail
	cmpl $0, statat_buffer + 32(%rip)
	jne fail
	cmpq $15, statat_buffer + 48(%rip)
	jne fail

	movq $332, %rax
	movq $-100, %rdi
	leaq hello_path(%rip), %rsi
	xorq %rdx, %rdx
	movq $0x7ff, %r10
	leaq statx_buffer(%rip), %r8
	syscall
	testq %rax, %rax
	jne fail
	cmpl $0x7ff, statx_buffer(%rip)
	jne fail
	cmpw $0100444, statx_buffer + 28(%rip)
	jne fail
	cmpq $15, statx_buffer + 40(%rip)
	jne fail

	movq $1, %rax
	movq $1, %rdi
	leaq stat_message(%rip), %rsi
	movq $stat_message_end - stat_message, %rdx
	syscall
	cmpq $stat_message_end - stat_message, %rax
	jne fail

	xorq %rax, %rax
	movq %r12, %rdi
	leaq read_buffer(%rip), %rsi
	movq $15, %rdx
	syscall
	cmpq $15, %rax
	jne fail

	movq $1, %rax
	movq $1, %rdi
	leaq read_buffer(%rip), %rsi
	movq $15, %rdx
	syscall
	cmpq $15, %rax
	jne fail

	xorq %rax, %rax
	movq %r12, %rdi
	leaq read_buffer(%rip), %rsi
	movq $15, %rdx
	syscall
	testq %rax, %rax
	jne fail

	movq $3, %rax
	movq %r12, %rdi
	syscall
	testq %rax, %rax
	jne fail

	xorq %rax, %rax
	movq %r12, %rdi
	leaq read_buffer(%rip), %rsi
	movq $1, %rdx
	syscall
	cmpq $-9, %rax
	jne fail

	movq $257, %rax
	movq $-100, %rdi
	leaq nested_dir_path(%rip), %rsi
	movq $00200000, %rdx
	syscall
	testq %rax, %rax
	js fail
	movq %rax, %r12

	movq $257, %rax
	movq %r12, %rdi
	leaq nested_leaf_path(%rip), %rsi
	xorq %rdx, %rdx
	syscall
	testq %rax, %rax
	js fail
	movq %rax, %r13

	xorq %rax, %rax
	movq %r13, %rdi
	leaq read_buffer(%rip), %rsi
	movq $19, %rdx
	syscall
	cmpq $19, %rax
	jne fail

	movq $3, %rax
	movq %r13, %rdi
	syscall
	testq %rax, %rax
	jne fail

	movq $3, %rax
	movq %r12, %rdi
	syscall
	testq %rax, %rax
	jne fail

	movq $1, %rax
	movq $1, %rdi
	leaq openat_message(%rip), %rsi
	movq $openat_message_end - openat_message, %rdx
	syscall
	cmpq $openat_message_end - openat_message, %rax
	jne fail

	movl $2, group_buffer(%rip)
	movl $3, group_buffer + 4(%rip)
	movq $116, %rax
	movq $2, %rdi
	leaq group_buffer(%rip), %rsi
	syscall
	testq %rax, %rax
	jne fail
	movq $115, %rax
	movq $2, %rdi
	leaq group_buffer(%rip), %rsi
	syscall
	cmpq $2, %rax
	jne fail
	cmpl $2, group_buffer(%rip)
	jne fail
	cmpl $3, group_buffer + 4(%rip)
	jne fail

	movq $119, %rax
	movq $5, %rdi
	movq $6, %rsi
	movq $7, %rdx
	syscall
	testq %rax, %rax
	jne fail
	movq $104, %rax
	syscall
	cmpq $5, %rax
	jne fail
	movq $108, %rax
	syscall
	cmpq $6, %rax
	jne fail

	movq $117, %rax
	movq $8, %rdi
	movq $9, %rsi
	movq $10, %rdx
	syscall
	testq %rax, %rax
	jne fail
	movq $102, %rax
	syscall
	cmpq $8, %rax
	jne fail
	movq $107, %rax
	syscall
	cmpq $9, %rax
	jne fail

	movq $105, %rax
	xorq %rdi, %rdi
	syscall
	cmpq $-1, %rax
	jne fail
	movq $106, %rax
	xorq %rdi, %rdi
	syscall
	cmpq $-1, %rax
	jne fail

	movq $21, %rax
	leaq hello_path(%rip), %rdi
	movq $4, %rsi
	syscall
	testq %rax, %rax
	jne fail
	movq $269, %rax
	movq $-100, %rdi
	leaq exec_path(%rip), %rsi
	movq $1, %rdx
	syscall
	testq %rax, %rax
	jne fail
	movq $439, %rax
	movq $-100, %rdi
	leaq exec_path(%rip), %rsi
	movq $1, %rdx
	xorq %r10, %r10
	syscall
	testq %rax, %rax
	jne fail
	movq $21, %rax
	leaq secret_path(%rip), %rdi
	movq $4, %rsi
	syscall
	cmpq $-13, %rax
	jne fail

	movq $257, %rax
	movq $-100, %rdi
	leaq secret_path(%rip), %rsi
	xorq %rdx, %rdx
	syscall
	cmpq $-13, %rax
	jne fail

	movq $1, %rax
	movq $1, %rdi
	leaq credential_mutation_message(%rip), %rsi
	movq $credential_mutation_message_end - credential_mutation_message, %rdx
	syscall
	cmpq $credential_mutation_message_end - credential_mutation_message, %rax
	jne fail

	movq $231, %rax
	xorq %rdi, %rdi
	syscall

fail:
	movq $1, %rax
	movq $2, %rdi
	leaq fail_message(%rip), %rsi
	movq $fail_message_end - fail_message, %rdx
	syscall

	movq $231, %rax
	movq $1, %rdi
	syscall

.halt:
	jmp .halt

.section .rodata, "a"
message:
	.ascii "linux syscall shared buffer\n"
message_end:
badfd_message:
	.ascii "linux bad fd should not print\n"
badfd_message_end:
ok_message:
	.ascii "linux return checks ok\n"
ok_message_end:
credential_message:
	.ascii "linux credential checks ok\n"
credential_message_end:
credential_mutation_message:
	.ascii "linux credential mutation checks ok\n"
credential_mutation_message_end:
brk_message:
	.ascii "linux brk checks ok\n"
brk_message_end:
stat_message:
	.ascii "linux metadata checks ok\n"
stat_message_end:
mmap_message:
	.ascii "linux mmap checks ok\n"
mmap_message_end:
wait_message:
	.ascii "linux wait checks ok\n"
wait_message_end:
fork_child_message:
	.ascii "linux fork child ok\n"
fork_child_message_end:
fork_credential_message:
	.ascii "linux fork credential checks ok\n"
fork_credential_message_end:
munmap_message:
	.ascii "linux munmap checks ok\n"
munmap_message_end:
openat_message:
	.ascii "linux openat dirfd checks ok\n"
openat_message_end:
fail_message:
	.ascii "linux return checks failed\n"
fail_message_end:
hello_path:
	.asciz "/hello.txt"
nested_dir_path:
	.asciz "/usr/share/bunix/nested"
nested_leaf_path:
	.asciz "hello.txt"
secret_path:
	.asciz "/secret.txt"
exec_path:
	.asciz "/bin/execok"

.section .bss, "aw"
read_buffer:
	.skip 32
stat_buffer:
	.skip 144
statat_buffer:
	.skip 144
statx_buffer:
	.skip 256
wait_status:
	.skip 4
group_buffer:
	.skip 8
