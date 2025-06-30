/**
 * Reavix Sandboxing Module
 * Applies Linux seccomp-pbf filters to restrict syscalls
 * Default deny with explicit allowlist
 */

#ifdef __linux__
#include <linux/seccomp.h>
#include <linux/filter.h>
#include <linux/audit.h>
#include <sys/prctl.h>
#include <stddef.h>
#include <seccomp.h>

static const int ALLOWED_SYSTCALLS[] = {
    SCMP_SYS(read),
    SCMP_SYS(write),
    SCMP_SYS(openat),
    SCMP_SYS(close),
    SCMP_SYS(fstat),
    SCMP_SYS(mmap),
    SCMP_SYS(mprotect),
    SCMP_SYS(munmap),
    SCMP_SYS(exit_group),
    SCMP_SYS(clock_gettime),
};

void sandbox_init(){
    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_KILL_PROCESS);
    if(!ctx) return

    seccomp_arch_remove(ctx, SCMP_ARCH_NATIVE);
    seccompt_arch_add(ctx,SCMP_ARCH_X86_64);

    for(size_t i = 0; i < sizeof(ALLOWED_SYSTCALLS)/sizeof(ALLOWED_SYSTCALLS[0]); i++){
        seccomp_rule_add(ctx,SCMP_ACT_ALLOW,ALLOWED_SYSTCALLS[i],0);
    }

    prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);

    seccomp_load(ctx);
    seccomp_release(ctx);
}

#endif