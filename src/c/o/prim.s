 .data
 .balign 4,0x0
 .long 0
 .text
 .balign 4
.globl return_from_signal_handler_asm ; .align 2 ; return_from_signal_handler_asm:
 movl $(((0)*2)+1),72(%esp)
 movl $(((0)*2)+1),76(%esp)
 movl $(((0)*2)+1),16(%esp)
 movl $11,%eax
 jmp set_request
.globl resume_after_handling_signal ; resume_after_handling_signal:
 movl $12,%eax
 jmp set_request
.globl return_from_software_generated_periodic_event_handler_asm ; .align 2 ; return_from_software_generated_periodic_event_handler_asm:
 movl $(((0)*2)+1),72(%esp)
 movl $(((0)*2)+1),76(%esp)
 movl $(((0)*2)+1),16(%esp)
 movl $13,%eax
 jmp set_request
.globl resume_after_handling_software_generated_periodic_event ; resume_after_handling_software_generated_periodic_event:
 movl $14,%eax
 jmp set_request
.globl handle_uncaught_exception_closure_asm ; .align 2 ; handle_uncaught_exception_closure_asm:
 movl 72(%esp),%eax; movl %eax,16(%esp)
 movl $2,%eax
 jmp set_request
.globl return_to_c_level_asm ; .align 2 ; return_to_c_level_asm:
 movl $(((0)*2)+1),72(%esp)
 movl $(((0)*2)+1),76(%esp)
 movl $(((0)*2)+1),16(%esp)
 movl $1,%eax
 jmp set_request
.globl request_fault ; request_fault:
 call FPEEnable
 movl 72(%esp),%eax; movl %eax,16(%esp)
 movl $3,%eax
 jmp set_request
.globl find_cfun_asm ; .align 2 ; find_cfun_asm:
 1:; movl 72(%esp),%eax; movl %eax,16(%esp); cmpl 12(%esp),%edi; jb 9f; call call_heapcleaner_asm; jmp 1b; 9:
 movl $4,%eax
 jmp set_request
.globl make_package_literals_via_bytecode_interpreter_asm ; .align 2 ; make_package_literals_via_bytecode_interpreter_asm:
 1:; movl 72(%esp),%eax; movl %eax,16(%esp); cmpl 12(%esp),%edi; jb 9f; call call_heapcleaner_asm; jmp 1b; 9:
 movl $15,%eax
 jmp set_request
.globl call_cfun_asm ; .align 2 ; call_cfun_asm:
 1:; movl 72(%esp),%eax; movl %eax,16(%esp); cmpl 12(%esp),%edi; jb 9f; call call_heapcleaner_asm; jmp 1b; 9:
 movl $5,%eax
 jmp set_request
.globl call_heapcleaner_asm ; call_heapcleaner_asm:
 popl 16(%esp)
 movl $0,%eax
.globl set_request ; set_request:
 movl %eax,504(%esp)
 movl 176(%esp),%eax
 movl %edi,12(%eax)
 movl %ebp,24(%eax)
 movl %esi,28(%eax)
 movl 4(%eax),%edi
 movl $0,8(%edi)
 movl %ebx,52(%eax)
 movl %ecx,56(%eax)
 movl %edx,60(%eax)
 movl 12(%esp),%edi; movl %edi,16(%eax)
 movl 8(%esp),%edi; movl %edi,44(%eax)
 movl 76(%esp),%edi; movl %edi,32(%eax)
 movl 72(%esp),%edi; movl %edi,36(%eax)
 movl 16(%esp),%edi; movl %edi,40(%eax)
 movl 24(%esp),%edi; movl %edi,64(%eax)
 movl 28(%esp),%edi; movl %edi,48(%eax)
 movl 504(%esp),%eax
 movl 500(%esp),%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
 .text
 .balign 4
.globl asm_run_mythryl_task ; asm_run_mythryl_task:
 movl 4(%esp),%eax
 pushl %ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movl %esp,%ebx
 orl $4,%esp
 subl $4,%esp
 subl $(8192),%esp
 movl %ebx,500(%esp)
 movl 44(%eax),%ebx; movl %ebx,8(%esp)
 movl 16(%eax),%ebx; movl %ebx,12(%esp)
 movl 64(%eax),%ebx; movl %ebx,24(%esp)
 movl 48(%eax),%ebx; movl %ebx,28(%esp)
 leal call_heapcleaner_asm,%ebx
 movl %ebx,32(%esp)
 movl %eax,176(%esp)
 movl 36(%eax),%ebx; movl %ebx,72(%esp)
 movl 32(%eax),%ebx; movl %ebx,76(%esp)
 movl 40(%eax),%ebx; movl %ebx,16(%esp)
 movl 12(%eax),%edi
 movl 28(%eax),%esi
 movl 24(%eax),%ebp
 movl 52(%eax),%ebx
 movl 56(%eax),%ecx
 movl 60(%eax),%edx
 movl %esp,76(%eax)
 pushl %edx
 pushl %eax
 movl 4(%eax),%eax
 movl $1,8(%eax)
 movl 20(%eax),%edx
 cmpl 24(%eax),%edx
 jne pending
restore_and_run_mythryl_code:
 popl %eax
 popl %edx
run_mythryl_code:
 cmpl 12(%esp),%edi
 jmp *40(%eax)
pending:
 cmpl $0,16(%eax)
 jne restore_and_run_mythryl_code
 movl $1, 12( %eax )
 popl %eax
 popl %edx
 movl %edi,12(%esp)
 jmp run_mythryl_code
.globl make_typeagnostic_rw_vector_asm ; .align 2 ; make_typeagnostic_rw_vector_asm:
 1:; movl 72(%esp),%eax; movl %eax,16(%esp); cmpl 12(%esp),%edi; jb 9f; call call_heapcleaner_asm; jmp 1b; 9:
 movl (%ebp),%eax
 sarl $1,%eax
 cmpl $512,%eax
 jge 3f
 pushl %ebx
 pushl %ecx
 movl %eax,%ebx
 sall $(2 +5),%ebx
 orl $(((0x3)*4) + 0x2),%ebx
 movl %ebx,(%edi)
 addl $4,%edi
 movl %edi,%ebx
 movl 4(%ebp),%ecx
2:
 movl %ecx,(%edi)
 addl $4,%edi
 subl $1,%eax
 jne 2b
 movl $(((0x0)*128) + (((0x2)*4) + 0x2)),(%edi)
 addl $4,%edi
 movl (%ebp),%eax
 movl %edi,%ebp
 movl %ebx,(%edi)
 movl %eax,4(%edi)
 addl $8,%edi
 popl %ecx
 popl %ebx
 jmp *%esi
3:
 movl 72(%esp),%eax; movl %eax,16(%esp)
 movl $9,%eax
 jmp set_request
.globl make_float64_rw_vector_asm ; .align 2 ; make_float64_rw_vector_asm:
 1:; movl 72(%esp),%eax; movl %eax,16(%esp); cmpl 12(%esp),%edi; jb 9f; call call_heapcleaner_asm; jmp 1b; 9:
        pushl %ebx
 movl %ebp,%eax
 sarl $1,%eax
 shll $1,%eax
 cmpl $512,%eax
 jge 2f
 orl $4,%edi
 movl %eax,%ebx
 shll $(2 +5),%ebx
 orl $(((0x5)*4) + 0x2),%ebx
 movl %ebx,(%edi)
 addl $4,%edi
 movl %edi,%ebx
 shll $2,%eax
 addl %eax,%edi
 movl $(((0x6)*128) + (((0x2)*4) + 0x2)),(%edi)
 addl $4,%edi
 movl %ebx,(%edi)
 movl %ebp,4(%edi)
 movl %edi,%ebp
 addl $8,%edi
 popl %ebx
 jmp *%esi
2:
 popl %ebx
 movl 72(%esp),%eax; movl %eax,16(%esp)
 movl $8,%eax
 jmp set_request
.globl make_unt8_rw_vector_asm ; .align 2 ; make_unt8_rw_vector_asm:
 1:; movl 72(%esp),%eax; movl %eax,16(%esp); cmpl 12(%esp),%edi; jb 9f; call call_heapcleaner_asm; jmp 1b; 9:
 movl %ebp,%eax
 sarl $1,%eax
 addl $3,%eax
 sarl $2,%eax
 cmpl $512,%eax
 jmp 2f
 jge 2f
 pushl %ebx
 movl %eax,%ebx
 shll $(2 +5),%ebx
 orl $(((0x4)*4) + 0x2),%ebx
 movl %ebx,(%edi)
 addl $4,%edi
 movl %edi,%ebx
 shll $2,%eax
 addl %eax,%edi
 movl $(((0x1)*128) + (((0x2)*4) + 0x2)),(%edi)
 addl $4,%edi
 movl %ebx,(%edi)
 movl %ebp,4(%edi)
 movl %edi,%ebp
 addl $8,%edi
 popl %ebx
 jmp *%esi
2:
 movl 72(%esp),%eax; movl %eax,16(%esp)
 movl $7,%eax
 jmp set_request
.globl make_string_asm ; .align 2 ; make_string_asm:
 1:; movl 72(%esp),%eax; movl %eax,16(%esp); cmpl 12(%esp),%edi; jb 9f; call call_heapcleaner_asm; jmp 1b; 9:
 movl %ebp,%eax
 sarl $1,%eax
 addl $4,%eax
 sarl $2,%eax
 cmpl $512,%eax
 jge 2f
 pushl %ebx
 movl %eax,%ebx
 shll $(2 +5),%ebx
 orl $(((0x4)*4) + 0x2),%ebx
 movl %ebx,(%edi)
 addl $4,%edi
 movl %edi,%ebx
 shll $2,%eax
 addl %eax,%edi
 movl $0,-4(%edi)
 movl $(((0x1)*128) + (((0x1)*4) + 0x2)),%eax
 movl %eax,(%edi)
 addl $4,%edi
 movl %ebx,(%edi)
 movl %ebp,4(%edi)
 movl %edi,%ebp
 addl $8,%edi
 popl %ebx
 jmp *%esi
2:
 movl 72(%esp),%eax; movl %eax,16(%esp)
 movl $6,%eax
 jmp set_request
.globl make_vector_asm ; .align 2 ; make_vector_asm:
 1:; movl 72(%esp),%eax; movl %eax,16(%esp); cmpl 12(%esp),%edi; jb 9f; call call_heapcleaner_asm; jmp 1b; 9:
 pushl %ebx
 pushl %ecx
 movl (%ebp),%eax
 movl %eax,%ebx
 sarl $1,%ebx
 cmpl $512,%ebx
 jge 3f
 shll $(2 +5),%ebx
 orl $(((0x0)*4) + 0x2),%ebx
 movl %ebx,(%edi)
 addl $4,%edi
 movl 4(%ebp),%ebx
 movl %edi,%ebp
2:
 movl (%ebx),%ecx
 movl %ecx,(%edi)
 addl $4,%edi
 movl 4(%ebx),%ebx
 cmpl $(((0)*2)+1),%ebx
 jne 2b
 movl $(((0x0)*128) + (((0x1)*4) + 0x2)),%ebx
 movl %ebx,(%edi)
 addl $4,%edi
 movl %ebp,(%edi)
 movl %eax,4(%edi)
 movl %edi,%ebp
 addl $8,%edi
 popl %ecx
 popl %ebx
 jmp *%esi
3:
 popl %ecx
 popl %ebx
 movl 72(%esp),%eax; movl %eax,16(%esp)
 movl $10,%eax
 jmp set_request
.globl try_lock_asm ; .align 2 ; try_lock_asm:
 movl (%ebp),%eax
 movl $1,(%ebp)
 movl %eax,%ebp
 jmp *%esi
.globl unlock_asm ; .align 2 ; unlock_asm:
 movl $3,(%ebp)
 movl $1,%ebp
 jmp *%esi
 .data
 .align 2
old_controlwd:
 .word 0
new_controlwd:
 .word 0
 .text
 .align 2
.globl FPEEnable ; FPEEnable:
 wait; fninit
 subl $4,%esp
 wait; fnstcw (%esp)
 andw $0xf0c0,(%esp)
 orw $0x023f,(%esp)
 fldcw (%esp)
 addl $4,%esp
 ret
.globl fegetround ; fegetround:
 subl $4,%esp
 wait; fnstcw (%esp)
 sarl $10,(%esp)
 andl $3,(%esp)
 movl (%esp),%eax
 addl $4,%esp
 ret
.globl fesetround ; fesetround:
 subl $4,%esp
 wait; fnstcw (%esp)
 andw $0xf3ff,(%esp)
 movl 8(%esp),%eax
 sall $10,%eax
 orl %eax,(%esp)
 fldcw (%esp)
 addl $4,%esp
 ret
.globl floor_asm ; .align 2 ; floor_asm:
 wait; fnstcw old_controlwd
 movw old_controlwd,%ax
 andw $0xf3ff,%ax
 orw $0x0400,%ax
 movw %ax,new_controlwd
 fldcw new_controlwd
 fldl (%ebp)
 subl $4,%esp
 fistpl (%esp)
 popl %ebp
 sall $1,%ebp
 incl %ebp
 fldcw old_controlwd
 jmp *%esi
.globl logb_asm ; .align 2 ; logb_asm:
 movl 4(%ebp),%eax
 sarl $20,%eax
 andl $0x7ff,%eax
 subl $1023,%eax
 sall $1,%eax
 addl $1,%eax
 movl %eax,%ebp
 jmp *%esi
.globl scalb_asm ; .align 2 ; scalb_asm:
 1:; movl 72(%esp),%eax; movl %eax,16(%esp); cmpl 12(%esp),%edi; jb 9f; call call_heapcleaner_asm; jmp 1b; 9:
 pushl 4(%ebp)
 sarl $1,(%esp)
 fildl (%esp)
 movl (%ebp),%eax
 fldl (%eax)
 fscale
 movl $(((2)*128) + (((0x5)*4) + 0x2)),(%edi)
 fstpl 4(%edi)
 addl $4,%edi
 movl %edi,%ebp
 addl $8,%edi
 fstpl (%esp)
 addl $4,%esp
 jmp *%esi
