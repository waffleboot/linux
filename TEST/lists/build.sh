#MACROS="-E"

MACROS="$MACROS -D__i386__ -DCONFIG_X86_32 -DCONFIG_X86_L1_CACHE_SHIFT=5"

rm a.out
gcc $MACROS -D__KERNEL__ -I ../../include -I . test.c
