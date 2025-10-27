	.file	"rename-1.c"
	.text
	.p2align 4
	.globl	f
	.type	f, @function
f:
.LFB0:
	.cfi_startproc
	movl	%edi, %eax
	ret
	.cfi_endproc
.LFE0:
	.size	f, .-f
	.ident	"GCC: (GNU) 14.2.1 20240927"
	.section	.note.GNU-stack,"",@progbits
