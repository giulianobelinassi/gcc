	.file	"rename-2.c"
	.text
	.p2align 4
	.type	g, @function
g:
.LFB0:
	.cfi_startproc
	movl	$3, -4(%rsp)
	movl	-4(%rsp), %eax
	ret
	.cfi_endproc
.LFE0:
	.size	g, .-g
	.p2align 4
	.globl	f
	.type	f, @function
f:
.LFB1:
	.cfi_startproc
	xorl	%eax, %eax
	jmp	g
	.cfi_endproc
.LFE1:
	.size	f, .-f
	.ident	"GCC: (GNU) 14.2.1 20240927"
	.section	.note.GNU-stack,"",@progbits
