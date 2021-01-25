cmd_/home/sjkim/Desktop/hypervisor-fuzz/utils/modgen/mycodes/4_dma/compileroom/mod.mod.o := gcc -Wp,-MMD,/home/sjkim/Desktop/hypervisor-fuzz/utils/modgen/mycodes/4_dma/compileroom/.mod.mod.o.d  -nostdinc -isystem /usr/lib/gcc/x86_64-linux-gnu/7/include -I/home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include -I./arch/x86/include/generated -I/home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include -I./include -I/home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/uapi -I./arch/x86/include/generated/uapi -I/home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi -I./include/generated/uapi -include /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/kconfig.h -include /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/compiler_types.h -D__KERNEL__ -Wall -Wundef -Werror=strict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -fno-PIE -Werror=implicit-function-declaration -Werror=implicit-int -Wno-format-security -std=gnu89 -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -mno-avx -m32 -msoft-float -mregparm=3 -freg-struct-return -fno-pic -mpreferred-stack-boundary=2 -march=i686 -mtune=generic -Wa,-mtune=generic32 -ffreestanding -Wno-sign-compare -fno-asynchronous-unwind-tables -mindirect-branch=thunk-extern -mindirect-branch-register -fno-jump-tables -fno-delete-null-pointer-checks -Wno-frame-address -Wno-format-truncation -Wno-format-overflow -O2 --param=allow-store-data-races=0 -Wframe-larger-than=1024 -fstack-protector-strong -Wno-unused-but-set-variable -Wimplicit-fallthrough -Wno-unused-const-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-var-tracking-assignments -g -Wdeclaration-after-statement -Wvla -Wno-pointer-sign -Wno-array-bounds -Wno-stringop-overflow -Wno-restrict -Wno-maybe-uninitialized -fno-strict-overflow -fno-merge-all-constants -fmerge-constants -fno-stack-check -fconserve-stack -Werror=date-time -Werror=incompatible-pointer-types -Werror=designated-init  -DMODULE  -DKBUILD_BASENAME='"mod.mod"' -DKBUILD_MODNAME='"mod"' -c -o /home/sjkim/Desktop/hypervisor-fuzz/utils/modgen/mycodes/4_dma/compileroom/mod.mod.o /home/sjkim/Desktop/hypervisor-fuzz/utils/modgen/mycodes/4_dma/compileroom/mod.mod.c

source_/home/sjkim/Desktop/hypervisor-fuzz/utils/modgen/mycodes/4_dma/compileroom/mod.mod.o := /home/sjkim/Desktop/hypervisor-fuzz/utils/modgen/mycodes/4_dma/compileroom/mod.mod.c

deps_/home/sjkim/Desktop/hypervisor-fuzz/utils/modgen/mycodes/4_dma/compileroom/mod.mod.o := \
    $(wildcard include/config/module/unload.h) \
    $(wildcard include/config/retpoline.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/kconfig.h \
    $(wildcard include/config/cc/version/text.h) \
    $(wildcard include/config/cpu/big/endian.h) \
    $(wildcard include/config/booger.h) \
    $(wildcard include/config/foo.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/compiler_types.h \
    $(wildcard include/config/have/arch/compiler/h.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/cc/has/asm/inline.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/compiler_attributes.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/module.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/sysfs.h) \
    $(wildcard include/config/modules/tree/lookup.h) \
    $(wildcard include/config/livepatch.h) \
    $(wildcard include/config/unused/symbols.h) \
    $(wildcard include/config/module/sig.h) \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/kallsyms.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/tracepoints.h) \
    $(wildcard include/config/tree/srcu.h) \
    $(wildcard include/config/bpf/events.h) \
    $(wildcard include/config/jump/label.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/event/tracing.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
    $(wildcard include/config/kprobes.h) \
    $(wildcard include/config/constructors.h) \
    $(wildcard include/config/function/error/injection.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/types.h \
    $(wildcard include/config/have/uid16.h) \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/types.h \
  arch/x86/include/generated/uapi/asm/types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/asm-generic/types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/int-ll64.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/asm-generic/int-ll64.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/uapi/asm/bitsperlong.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/bitsperlong.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/asm-generic/bitsperlong.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/posix_types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/stddef.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/stddef.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/compiler_types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/posix_types.h \
    $(wildcard include/config/x86/32.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/uapi/asm/posix_types_32.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/asm-generic/posix_types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/poison.h \
    $(wildcard include/config/illegal/pointer/value.h) \
    $(wildcard include/config/page/poisoning/zero.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/const.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/vdso/const.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/const.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/preempt/rt.h) \
    $(wildcard include/config/mmu.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/panic/timeout.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/limits.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/limits.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/vdso/limits.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/linkage.h \
    $(wildcard include/config/arch/use/sym/annotations.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/stringify.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/export.h \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/module/rel/crcs.h) \
    $(wildcard include/config/have/arch/prel32/relocations.h) \
    $(wildcard include/config/trim/unused/ksyms.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/stack/validation.h) \
    $(wildcard include/config/debug/entry.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/barrier.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/alternative.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/asm.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/nops.h \
    $(wildcard include/config/mk7.h) \
    $(wildcard include/config/x86/p6/nop.h) \
    $(wildcard include/config/x86/64.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/barrier.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/kasan-checks.h \
    $(wildcard include/config/kasan.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/kcsan-checks.h \
    $(wildcard include/config/kcsan.h) \
    $(wildcard include/config/kcsan/ignore/atomics.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/linkage.h \
    $(wildcard include/config/x86/alignment/16.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/bitops.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/bits.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/vdso/bits.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/build_bug.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/bitops.h \
    $(wildcard include/config/x86/cmov.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/rmwcc.h \
    $(wildcard include/config/cc/has/asm/goto.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/bitops/fls64.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/bitops/find.h \
    $(wildcard include/config/generic/find/first/bit.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/bitops/sched.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/arch_hweight.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/cpufeatures.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/required-features.h \
    $(wildcard include/config/x86/minimum/cpu/family.h) \
    $(wildcard include/config/math/emulation.h) \
    $(wildcard include/config/x86/pae.h) \
    $(wildcard include/config/x86/cmpxchg64.h) \
    $(wildcard include/config/x86/use/3dnow.h) \
    $(wildcard include/config/matom.h) \
    $(wildcard include/config/paravirt.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/disabled-features.h \
    $(wildcard include/config/x86/smap.h) \
    $(wildcard include/config/x86/umip.h) \
    $(wildcard include/config/x86/intel/memory/protection/keys.h) \
    $(wildcard include/config/x86/5level.h) \
    $(wildcard include/config/page/table/isolation.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/bitops/const_hweight.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/bitops/instrumented-atomic.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/instrumented.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/bitops/instrumented-non-atomic.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/bitops/instrumented-lock.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/bitops/le.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/uapi/asm/byteorder.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/byteorder/little_endian.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/byteorder/little_endian.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/swab.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/swab.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/uapi/asm/swab.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/byteorder/generic.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/bitops/ext2-atomic-setbit.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/typecheck.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/printk.h \
    $(wildcard include/config/message/loglevel/default.h) \
    $(wildcard include/config/console/loglevel/default.h) \
    $(wildcard include/config/console/loglevel/quiet.h) \
    $(wildcard include/config/early/printk.h) \
    $(wildcard include/config/printk/nmi.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/dynamic/debug.h) \
    $(wildcard include/config/dynamic/debug/core.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/init.h \
    $(wildcard include/config/strict/kernel/rwx.h) \
    $(wildcard include/config/strict/module/rwx.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/kern_levels.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/cache.h \
    $(wildcard include/config/arch/has/cache/line/size.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/kernel.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/sysinfo.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/cache.h \
    $(wildcard include/config/x86/l1/cache/shift.h) \
    $(wildcard include/config/x86/internode/cache/shift.h) \
    $(wildcard include/config/x86/vsmp.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/div64.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/stat.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/uapi/asm/stat.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/stat.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/time.h \
    $(wildcard include/config/arch/uses/gettimeoffset.h) \
    $(wildcard include/config/posix/timers.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/seqlock.h \
    $(wildcard include/config/debug/lock/alloc.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/spinlock.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/preemption.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/preempt.h \
    $(wildcard include/config/preempt/count.h) \
    $(wildcard include/config/debug/preempt.h) \
    $(wildcard include/config/trace/preempt/toggle.h) \
    $(wildcard include/config/preempt/notifiers.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/preempt.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/percpu.h \
    $(wildcard include/config/x86/64/smp.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/percpu.h \
    $(wildcard include/config/have/setup/per/cpu/area.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/percpu-defs.h \
    $(wildcard include/config/debug/force/weak/per/cpu.h) \
    $(wildcard include/config/amd/mem/encrypt.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/thread_info.h \
    $(wildcard include/config/thread/info/in/task.h) \
    $(wildcard include/config/have/arch/within/stack/frames.h) \
    $(wildcard include/config/hardened/usercopy.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/bug.h \
    $(wildcard include/config/bug/on/data/corruption.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/bug.h \
    $(wildcard include/config/debug/bugverbose.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/bug.h \
    $(wildcard include/config/bug.h) \
    $(wildcard include/config/generic/bug/relative/pointers.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/restart_block.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/time64.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/math64.h \
    $(wildcard include/config/arch/supports/int128.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/vdso/math64.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/vdso/time64.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/time.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/time_types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/current.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/thread_info.h \
    $(wildcard include/config/vm86.h) \
    $(wildcard include/config/x86/iopl/ioperm.h) \
    $(wildcard include/config/frame/pointer.h) \
    $(wildcard include/config/compat.h) \
    $(wildcard include/config/ia32/emulation.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/page.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/page_types.h \
    $(wildcard include/config/physical/start.h) \
    $(wildcard include/config/physical/align.h) \
    $(wildcard include/config/dynamic/physical/mask.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/mem_encrypt.h \
    $(wildcard include/config/arch/has/mem/encrypt.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/mem_encrypt.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/uapi/asm/bootparam.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/screen_info.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/screen_info.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/apm_bios.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/apm_bios.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/ioctl.h \
  arch/x86/include/generated/uapi/asm/ioctl.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/ioctl.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/asm-generic/ioctl.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/edd.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/edd.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/ist.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/uapi/asm/ist.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/video/edid.h \
    $(wildcard include/config/x86.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/video/edid.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/page_32_types.h \
    $(wildcard include/config/highmem4g.h) \
    $(wildcard include/config/highmem64g.h) \
    $(wildcard include/config/page/offset.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/page_32.h \
    $(wildcard include/config/debug/virtual.h) \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/x86/3dnow.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
    $(wildcard include/config/fortify/source.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/string.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/string.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/string_32.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/range.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/memory_model.h \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
    $(wildcard include/config/sparsemem.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/pfn.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/getorder.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/cpufeature.h \
    $(wildcard include/config/x86/feature/names.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/processor.h \
    $(wildcard include/config/x86/vmx/feature/names.h) \
    $(wildcard include/config/kvm.h) \
    $(wildcard include/config/stackprotector.h) \
    $(wildcard include/config/paravirt/xxl.h) \
    $(wildcard include/config/x86/debugctlmsr.h) \
    $(wildcard include/config/cpu/sup/amd.h) \
    $(wildcard include/config/xen.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/processor-flags.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/uapi/asm/processor-flags.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/math_emu.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/ptrace.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/segment.h \
    $(wildcard include/config/xen/pv.h) \
    $(wildcard include/config/x86/32/lazy/gs.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/uapi/asm/ptrace.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/uapi/asm/ptrace-abi.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/paravirt_types.h \
    $(wildcard include/config/pgtable/levels.h) \
    $(wildcard include/config/paravirt/debug.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/desc_defs.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/kmap_types.h \
    $(wildcard include/config/debug/highmem.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/kmap_types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/pgtable_types.h \
    $(wildcard include/config/mem/soft/dirty.h) \
    $(wildcard include/config/have/arch/userfaultfd/wp.h) \
    $(wildcard include/config/proc/fs.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/pgtable_32_types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/pgtable-2level_types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/pgtable-nop4d.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/pgtable-nopud.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/pgtable-nopmd.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/nospec-branch.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/static_key.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/jump_label.h \
    $(wildcard include/config/have/arch/jump/label/relative.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/jump_label.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/frame.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/alternative-asm.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/msr-index.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/unwind_hints.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/orc_types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/spinlock_types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/qspinlock_types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/qrwlock_types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/uapi/asm/sigcontext.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/msr.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/msr-index.h \
  arch/x86/include/generated/uapi/asm/errno.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/asm-generic/errno.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/asm-generic/errno-base.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/cpumask.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/cpumask.h \
    $(wildcard include/config/cpumask/offstack.h) \
    $(wildcard include/config/hotplug/cpu.h) \
    $(wildcard include/config/debug/per/cpu/maps.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/bitmap.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/atomic.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/atomic.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/cmpxchg.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/cmpxchg_32.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/atomic64_32.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/atomic-arch-fallback.h \
    $(wildcard include/config/generic/atomic64.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/atomic-instrumented.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/atomic-long.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/uapi/asm/msr.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/tracepoint-defs.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/errno.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/errno.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/special_insns.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/irqflags.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/fpu/types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/vmxfeatures.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/vdso/processor.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/personality.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/personality.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/err.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/bottom_half.h \
  arch/x86/include/generated/asm/mmiowb.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/mmiowb.h \
    $(wildcard include/config/mmiowb.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/spinlock_types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/lockdep.h \
    $(wildcard include/config/prove/raw/lock/nesting.h) \
    $(wildcard include/config/preempt/lock.h) \
    $(wildcard include/config/lockdep.h) \
    $(wildcard include/config/lock/stat.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/rwlock_types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/spinlock.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/paravirt.h \
    $(wildcard include/config/paravirt/spinlocks.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/frame.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/qspinlock.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/qspinlock.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/qrwlock.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/qrwlock.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/rwlock.h \
    $(wildcard include/config/preempt.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/spinlock_api_smp.h \
    $(wildcard include/config/inline/spin/lock.h) \
    $(wildcard include/config/inline/spin/lock/bh.h) \
    $(wildcard include/config/inline/spin/lock/irq.h) \
    $(wildcard include/config/inline/spin/lock/irqsave.h) \
    $(wildcard include/config/inline/spin/trylock.h) \
    $(wildcard include/config/inline/spin/trylock/bh.h) \
    $(wildcard include/config/uninline/spin/unlock.h) \
    $(wildcard include/config/inline/spin/unlock/bh.h) \
    $(wildcard include/config/inline/spin/unlock/irq.h) \
    $(wildcard include/config/inline/spin/unlock/irqrestore.h) \
    $(wildcard include/config/generic/lockbreak.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/rwlock_api_smp.h \
    $(wildcard include/config/inline/read/lock.h) \
    $(wildcard include/config/inline/write/lock.h) \
    $(wildcard include/config/inline/read/lock/bh.h) \
    $(wildcard include/config/inline/write/lock/bh.h) \
    $(wildcard include/config/inline/read/lock/irq.h) \
    $(wildcard include/config/inline/write/lock/irq.h) \
    $(wildcard include/config/inline/read/lock/irqsave.h) \
    $(wildcard include/config/inline/write/lock/irqsave.h) \
    $(wildcard include/config/inline/read/trylock.h) \
    $(wildcard include/config/inline/write/trylock.h) \
    $(wildcard include/config/inline/read/unlock.h) \
    $(wildcard include/config/inline/write/unlock.h) \
    $(wildcard include/config/inline/read/unlock/bh.h) \
    $(wildcard include/config/inline/write/unlock/bh.h) \
    $(wildcard include/config/inline/read/unlock/irq.h) \
    $(wildcard include/config/inline/write/unlock/irq.h) \
    $(wildcard include/config/inline/read/unlock/irqrestore.h) \
    $(wildcard include/config/inline/write/unlock/irqrestore.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/time32.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/timex.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/timex.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/param.h \
  arch/x86/include/generated/uapi/asm/param.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/param.h \
    $(wildcard include/config/hz.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/asm-generic/param.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/timex.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/tsc.h \
    $(wildcard include/config/x86/tsc.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/vdso/time32.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/vdso/time.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/uidgid.h \
    $(wildcard include/config/multiuser.h) \
    $(wildcard include/config/user/ns.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/highuid.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/kmod.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/umh.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/gfp.h \
    $(wildcard include/config/highmem.h) \
    $(wildcard include/config/zone/dma.h) \
    $(wildcard include/config/zone/dma32.h) \
    $(wildcard include/config/zone/device.h) \
    $(wildcard include/config/numa.h) \
    $(wildcard include/config/pm/sleep.h) \
    $(wildcard include/config/contig/alloc.h) \
    $(wildcard include/config/cma.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/mmdebug.h \
    $(wildcard include/config/debug/vm.h) \
    $(wildcard include/config/debug/vm/pgflags.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/mmzone.h \
    $(wildcard include/config/force/max/zoneorder.h) \
    $(wildcard include/config/memory/isolation.h) \
    $(wildcard include/config/shadow/call/stack.h) \
    $(wildcard include/config/zsmalloc.h) \
    $(wildcard include/config/memcg.h) \
    $(wildcard include/config/memory/hotplug.h) \
    $(wildcard include/config/compaction.h) \
    $(wildcard include/config/transparent/hugepage.h) \
    $(wildcard include/config/flat/node/mem/map.h) \
    $(wildcard include/config/page/extension.h) \
    $(wildcard include/config/deferred/struct/page/init.h) \
    $(wildcard include/config/have/memory/present.h) \
    $(wildcard include/config/have/memoryless/nodes.h) \
    $(wildcard include/config/need/multiple/nodes.h) \
    $(wildcard include/config/sparsemem/extreme.h) \
    $(wildcard include/config/memory/hotremove.h) \
    $(wildcard include/config/have/arch/pfn/valid.h) \
    $(wildcard include/config/holes/in/zone.h) \
    $(wildcard include/config/arch/has/holes/memorymodel.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/wait.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/wait.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/numa.h \
    $(wildcard include/config/nodes/shift.h) \
    $(wildcard include/config/numa/keep/meminfo.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/nodemask.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/pageblock-flags.h \
    $(wildcard include/config/hugetlb/page.h) \
    $(wildcard include/config/hugetlb/page/size/variable.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/page-flags-layout.h \
    $(wildcard include/config/numa/balancing.h) \
    $(wildcard include/config/kasan/sw/tags.h) \
  include/generated/bounds.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/mm_types.h \
    $(wildcard include/config/have/aligned/struct/page.h) \
    $(wildcard include/config/userfaultfd.h) \
    $(wildcard include/config/swap.h) \
    $(wildcard include/config/have/arch/compat/mmap/bases.h) \
    $(wildcard include/config/membarrier.h) \
    $(wildcard include/config/aio.h) \
    $(wildcard include/config/mmu/notifier.h) \
    $(wildcard include/config/arch/want/batched/unmap/tlb/flush.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/mm_types_task.h \
    $(wildcard include/config/split/ptlock/cpus.h) \
    $(wildcard include/config/arch/enable/split/pmd/ptlock.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/tlbbatch.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/auxvec.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/auxvec.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/uapi/asm/auxvec.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/rbtree.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/rcupdate.h \
    $(wildcard include/config/preempt/rcu.h) \
    $(wildcard include/config/rcu/stall/common.h) \
    $(wildcard include/config/no/hz/full.h) \
    $(wildcard include/config/rcu/nocb/cpu.h) \
    $(wildcard include/config/tasks/rcu/generic.h) \
    $(wildcard include/config/tasks/rcu.h) \
    $(wildcard include/config/tasks/rcu/trace.h) \
    $(wildcard include/config/tasks/rude/rcu.h) \
    $(wildcard include/config/tree/rcu.h) \
    $(wildcard include/config/tiny/rcu.h) \
    $(wildcard include/config/debug/objects/rcu/head.h) \
    $(wildcard include/config/prove/rcu.h) \
    $(wildcard include/config/rcu/boost.h) \
    $(wildcard include/config/arch/weak/release/acquire.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/rcutree.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/rwsem.h \
    $(wildcard include/config/rwsem/spin/on/owner.h) \
    $(wildcard include/config/debug/rwsems.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/osq_lock.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/completion.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/swait.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/uprobes.h \
    $(wildcard include/config/uprobes.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/uprobes.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/notifier.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/mutex.h \
    $(wildcard include/config/mutex/spin/on/owner.h) \
    $(wildcard include/config/debug/mutexes.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/debug_locks.h \
    $(wildcard include/config/debug/locking/api/selftests.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/srcu.h \
    $(wildcard include/config/tiny/srcu.h) \
    $(wildcard include/config/srcu.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/workqueue.h \
    $(wildcard include/config/debug/objects/work.h) \
    $(wildcard include/config/freezer.h) \
    $(wildcard include/config/wq/watchdog.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/timer.h \
    $(wildcard include/config/debug/objects/timers.h) \
    $(wildcard include/config/no/hz/common.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/ktime.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/jiffies.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/vdso/jiffies.h \
  include/generated/timeconst.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/vdso/ktime.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/timekeeping.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/timekeeping32.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/debugobjects.h \
    $(wildcard include/config/debug/objects.h) \
    $(wildcard include/config/debug/objects/free.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/rcu_segcblist.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/srcutree.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/rcu_node_tree.h \
    $(wildcard include/config/rcu/fanout.h) \
    $(wildcard include/config/rcu/fanout/leaf.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/mmu.h \
    $(wildcard include/config/modify/ldt/syscall.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/page-flags.h \
    $(wildcard include/config/arch/uses/pg/uncached.h) \
    $(wildcard include/config/memory/failure.h) \
    $(wildcard include/config/idle/page/tracking.h) \
    $(wildcard include/config/thp/swap.h) \
    $(wildcard include/config/ksm.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/memory_hotplug.h \
    $(wildcard include/config/arch/has/add/pages.h) \
    $(wildcard include/config/have/arch/nodedata/extension.h) \
    $(wildcard include/config/have/bootmem/info/node.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/topology.h \
    $(wildcard include/config/use/percpu/numa/node/id.h) \
    $(wildcard include/config/sched/smt.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/arch_topology.h \
    $(wildcard include/config/generic/arch/topology.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/percpu.h \
    $(wildcard include/config/need/per/cpu/embed/first/chunk.h) \
    $(wildcard include/config/need/per/cpu/page/first/chunk.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/smp.h \
    $(wildcard include/config/up/late/init.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/smp_types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/llist.h \
    $(wildcard include/config/arch/have/nmi/safe/cmpxchg.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/smp.h \
    $(wildcard include/config/x86/local/apic.h) \
    $(wildcard include/config/x86/io/apic.h) \
    $(wildcard include/config/debug/nmi/selftest.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/mpspec.h \
    $(wildcard include/config/eisa.h) \
    $(wildcard include/config/x86/mpparse.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/mpspec_def.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/x86_init.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/apicdef.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/apic.h \
    $(wildcard include/config/x86/x2apic.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/fixmap.h \
    $(wildcard include/config/x86/vsyscall/emulation.h) \
    $(wildcard include/config/provide/ohci1394/dma/init.h) \
    $(wildcard include/config/pci/mmconfig.h) \
    $(wildcard include/config/x86/intel/mid.h) \
    $(wildcard include/config/acpi/apei/ghes.h) \
    $(wildcard include/config/intel/txt.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/acpi.h \
    $(wildcard include/config/acpi/apei.h) \
    $(wildcard include/config/acpi.h) \
    $(wildcard include/config/acpi/numa.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/acpi/pdc_intel.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/numa.h \
    $(wildcard include/config/numa/emu.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/topology.h \
    $(wildcard include/config/sched/mc/prio.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/topology.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/numa_32.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/fixmap.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/hardirq.h \
    $(wildcard include/config/kvm/intel.h) \
    $(wildcard include/config/have/kvm.h) \
    $(wildcard include/config/x86/thermal/vector.h) \
    $(wildcard include/config/x86/mce/threshold.h) \
    $(wildcard include/config/x86/mce/amd.h) \
    $(wildcard include/config/x86/hv/callback/vector.h) \
    $(wildcard include/config/hyperv.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/io_apic.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/irq_vectors.h \
    $(wildcard include/config/pci/msi.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/sysctl.h \
    $(wildcard include/config/sysctl.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/sysctl.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/elf.h \
    $(wildcard include/config/arch/use/gnu/property.h) \
    $(wildcard include/config/arch/have/elf/prot.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/elf.h \
    $(wildcard include/config/x86/x32/abi.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/user.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/user_32.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/fsgsbase.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/vdso.h \
    $(wildcard include/config/x86/x32.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/desc.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/uapi/asm/ldt.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/cpu_entry_area.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/intel_ds.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/pgtable_areas.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/pgtable_32_areas.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/elf.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/uapi/linux/elf-em.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/kobject.h \
    $(wildcard include/config/uevent/helper.h) \
    $(wildcard include/config/debug/kobject/release.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/sysfs.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/kernfs.h \
    $(wildcard include/config/kernfs.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/idr.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/radix-tree.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/xarray.h \
    $(wildcard include/config/xarray/multi.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/kconfig.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/local_lock.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/local_lock_internal.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/kobject_ns.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/kref.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/refcount.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/moduleparam.h \
    $(wildcard include/config/alpha.h) \
    $(wildcard include/config/ia64.h) \
    $(wildcard include/config/ppc64.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/rbtree_latch.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/error-injection.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/error-injection.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/module.h \
    $(wildcard include/config/unwinder/orc.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/asm-generic/module.h \
    $(wildcard include/config/have/mod/arch/specific.h) \
    $(wildcard include/config/modules/use/elf/rel.h) \
    $(wildcard include/config/modules/use/elf/rela.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/orc_types.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/build-salt.h \
    $(wildcard include/config/build/salt.h) \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/elfnote.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/include/linux/vermagic.h \
  include/generated/utsrelease.h \
  /home/sjkim/Desktop/hypervisor-fuzz/src/linux-5.8.0/arch/x86/include/asm/vermagic.h \
    $(wildcard include/config/m486sx.h) \
    $(wildcard include/config/m486.h) \
    $(wildcard include/config/m586.h) \
    $(wildcard include/config/m586tsc.h) \
    $(wildcard include/config/m586mmx.h) \
    $(wildcard include/config/mcore2.h) \
    $(wildcard include/config/m686.h) \
    $(wildcard include/config/mpentiumii.h) \
    $(wildcard include/config/mpentiumiii.h) \
    $(wildcard include/config/mpentiumm.h) \
    $(wildcard include/config/mpentium4.h) \
    $(wildcard include/config/mk6.h) \
    $(wildcard include/config/mk8.h) \
    $(wildcard include/config/melan.h) \
    $(wildcard include/config/mcrusoe.h) \
    $(wildcard include/config/mefficeon.h) \
    $(wildcard include/config/mwinchipc6.h) \
    $(wildcard include/config/mwinchip3d.h) \
    $(wildcard include/config/mcyrixiii.h) \
    $(wildcard include/config/mviac3/2.h) \
    $(wildcard include/config/mviac7.h) \
    $(wildcard include/config/mgeodegx1.h) \
    $(wildcard include/config/mgeode/lx.h) \

/home/sjkim/Desktop/hypervisor-fuzz/utils/modgen/mycodes/4_dma/compileroom/mod.mod.o: $(deps_/home/sjkim/Desktop/hypervisor-fuzz/utils/modgen/mycodes/4_dma/compileroom/mod.mod.o)

$(deps_/home/sjkim/Desktop/hypervisor-fuzz/utils/modgen/mycodes/4_dma/compileroom/mod.mod.o):
