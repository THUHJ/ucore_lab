# lab1实验 #
计22 黄杰 2012011272
***
[练习1]  
操作系统镜像文件ucore.img是如何一步一步生成的？(需要比较详细地解释Makefile中每一条相关命令和命令参数的含义，以及说明命令导致的结果)

> 分析markfile文件，首先先明确几个文件中利用到的函数，这些函数在function.mk中定义，主要有listf列出目录文件,toobj得到.o文件，todep得到.d文件，totarget添加目录得到完整路径名,packetname增加__objs__作为一个packet的名字，add_files_to_packet在packet中增加文件等。明确了以上函数有助于分析makefile主体文件中的操作历程。  
>  
> 为了生成ucore.img镜像，主要分三个步骤，第一个步骤得到kernel，第二个步骤得到bootblock，第三个步骤得到将这两部和合并生成ucore.img.  
> 第一步骤：得到kernel，执行的指令为
```
$(kernel): $(KOBJS)
	@echo + ld $@
	$(V)$(LD) $(LDFLAGS) -T tools/kernel.ld -o $@ $(KOBJS)
	@$(OBJDUMP) -S $@ > $(call asmfile,kernel)
	@$(OBJDUMP) -t $@ | $(SED) '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(call symfile,kernel)
```
> 为了生成kernel，我们需要KOBJS中的如下文件
> obj/kern/init/init.o obj/kern/libs/readline.o obj/kern/libs/stdio.o obj/kern/debug/kdebug.o obj/kern/debug/kmonitor.o obj/kern/debug/panic.o obj/kern/driver/clock.o obj/kern/driver/console.o obj/kern/driver/intr.o obj/kern/driver/picirq.o obj/kern/trap/trap.o obj/kern/trap/trapentry.o obj/kern/trap/vectors.o obj/kern/mm/pmm.o obj/libs/printfmt.o obj/libs/string.o  
> 这些.o文件通过
```$(call add_files_cc,$(call listf_cc,$(KSRCDIR)),kernel,$(KCFLAGS))```
这句语句从.c文件编译得到。其中KCFLAGS定义了gcc编译的参数，KSRCDIR表示了这些.c文件所在的具体目录.  
>有了这些文件之后，$(V)$(LD) $(LDFLAGS) -T tools/kernel.ld -o $@ $(KOBJS)这句指令生成kernel文件，实际语句中参数有：-m elf_i386 -nostdlib -T tools/kernel.ld -o bin/kernel。其中-T参数表示使用-T指定的脚本。-m是在64位机器上编译32位程序时使用的。 -nostdlib链接的时候不使用标准的系统启动文件和系统库。剩余两个语句是为了生成.asm和.smy文件。至此kernel文件生成。
>   
>第二个步骤生成bootblock
>实现语句如下
```$(bootblock): $(call toobj,$(bootfiles)) | $(call totarget,sign)
	|@echo + ld $@
	$(V)$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 $^ -o $(call toobj,bootblock)
	@$(OBJDUMP) -S $(call objfile,bootblock) > $(call asmfile,bootblock)
	@$(OBJDUMP) -t $(call objfile,bootblock) | $(SED) '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(call symfile,bootblock)
	@$(OBJCOPY) -S -O binary $(call objfile,bootblock) $(call outfile,bootblock)
	@$(call totarget,sign) $(call outfile,bootblock) $(bootblock)
```
>需要依赖的文件有bootasm.o、bootmain.o、sign。  
>生成bootasm.o、bootmain.o编译命令如下
```gcc -Iboot/ -fno-builtin -Wall -ggdb -m32 -gstabs -nostdinc  -fno-stack-protector -Ilibs/ -Os -nostdinc -c boot/bootasm.S -o obj/boot/bootasm.o
+ cc boot/bootmain.c
gcc -Iboot/ -fno-builtin -Wall -ggdb -m32 -gstabs -nostdinc  -fno-stack-protector -Ilibs/ -Os -nostdinc -c boot/bootmain.c -o obj/boot/bootmain.o
```
>其中一些参数的含义为-fno-builtin关闭内置函数功能，-gstabs+ 此选项以stabs格式声称调试信息,并且包含仅供gdb使用的额外调试信息. -ggdb 此选项将尽可能的生成gdb的可以使用的调试信息. -fno-stack-protector禁用堆栈保护
>生成sign的语句为
```
gcc -Itools/ -g -Wall -O2 -c tools/sign.c -o obj/sign/tools/sign.o
gcc -g -Wall -O2 obj/sign/tools/sign.o -o bin/sign
```
得到依赖文件后使用
$(V)$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 $^ -o $(call toobj,bootblock)
进行bootbolck，实际指令为ld -m elf_i386 -nostdlib -N -e start -Ttext 0x7C00 obj/boot/bootasm.o obj/boot/bootmain.o -o obj/bootblock.o。其中-N表示把text和data节设置为可读写。-Ttext把代码段的开始位置设置懂啊0x7C00。接下来的几句话将.o文件生成.sym，.asm,.out文件。最后条用sign工具生成bootblock.至此bootblck也得到了。
>  
>最后一步，是用得到的kernel和bootblock生成ucore.img。代码为:
```
$(UCOREIMG): $(kernel) $(bootblock)
	$(V)dd if=/dev/zero of=$@ count=10000
	$(V)dd if=$(bootblock) of=$@ conv=notrunc
	$(V)dd if=$(kernel) of=$@ seek=1 conv=notrunc
```
其中第一句是初始化的过程，将512字节都赋值为0，第二句将bootblock的内容放入其中，第三句话将kernel放入其中，其中if表示源文件，of表示目标文件，seek=1,表示备份时对of 后面的部分也就是目标文件跳过1块再开始写。即跳过了bootblock的位置。conv=notrunc表示不截断输出文件。至此最终的ucore.img就得到了。


一个被系统认为是符合规范的硬盘主引导扇区的特征是什么？
>根据sign工具是用来生成硬盘的主引导扇区的，从sign.c的代码段
```buf[510] = 0x55;
    buf[511] = 0xAA;
    FILE *ofp = fopen(argv[2], "wb+");
    size = fwrite(buf, 1, 512, ofp);
    if (size != 512) {
        fprintf(stderr, "write '%s' error, size is %d.\n", argv[2], size);
        return -1;
    }
```
>可以看出主引导扇区需要大小为512字节，且其中第510字节为0x55,第511字节为0xAA.(从第0个开始计数)


[练习2]
为了熟悉使用qemu和gdb进行的调试工作，我们进行如下的小练习：

1.从CPU加电后执行的第一条指令开始，单步跟踪BIOS的执行。
>从makefile中可以看到，使用make debug指令就能够使用带gdb的qemu的执行。且会首先使gdbinit中的指引。
>原有的代码为
```
file bin/kernel
target remote :1234
break kern_init
continue
```
设置断点后直接开始运行了，我们只需要修改为
```
set architecture i8086
target remote :1234
```
那么在make debug的时候就能够使其停止在开始的时候。然后使用stepi弹幕执行，使用x /i $pc输出当前的指令为什么。

2.在初始化位置0x7c00设置实地址断点,测试断点正常。
>可以在将gdbinit修改为
```
set architecture i8086
target remote :1234
b *0x7c00
continue
x /i $pc
```
经过测试断点正常。等够看到汇编指令为
```
==> 0x7c00:    cli  
    0x7c01:    cld
```

3.从0x7c00开始跟踪代码运行,将单步跟踪反汇编得到的代码与bootasm.S和 bootblock.asm进行比较。
>可以直接单步比较输出的代码与bootsam.s和bootblock.asm比较。也可以直接修改make debug中的语句
```
	debug: $(UCOREIMG)
		$(V)$(QEMU) -S -s -d in_asm -D $(BINDIR)/q.log -parallel stdio -hda $< -serial null
		$(V)sleep 2
		$(V)$(TERMINAL) -e "gdb -q -tui -x tools/gdbinit"
```
即增加了参数-d in_asm -D $(BINDIR)/q.log生成一个log文件，通过比对可以发现两者的汇编代码是一致的。

4.自己找一个bootloader或内核中的代码位置，设置断点并进行测试。
>更改gitinit为
```
set architecture i8086
target remote :1234
b *0x7c80
continue
x /i $pc
```
在 0x7c80位置设置断点输出的语句为
0x7c80:  and $0xffc0,%ax

[练习3]BIOS将通过读取硬盘主引导扇区到内存，并转跳到对应内存中的位置执行bootloader。请分析bootloader是如何完成从实模式进入保护模式的。
>分析bootasm.s代码可以了解模式转化的过程。转换过程主要分如下几个部分。  
>首先：初始化
>
```
start:
.code16                                             # Assemble for 16-bit mode
    cli                                             # Disable interrupts
    cld                                             # String operations increment
    # Set up the important data segment registers (DS, ES, SS).
    xorw %ax, %ax                                   # Segment number zero
    movw %ax, %ds                                   # -> Data Segment
    movw %ax, %es                                   # -> Extra Segment
    movw %ax, %ss                                   # -> Stack Segment
```
>主要作用是对一些段寄存器的初始化。  
>第二步：开启A20
```
seta20.1:
    inb $0x64, %al                                  # Wait for not busy(8042 input buffer empty).
    testb $0x2, %al
    jnz seta20.1
    movb $0xd1, %al                                 # 0xd1 -> port 0x64
    outb %al, $0x64                                 # 0xd1 means: write data to 8042's P2 port
seta20.2:
    inb $0x64, %al                                  # Wait for not busy(8042 input buffer empty).
    testb $0x2, %al
    jnz seta20.2
    movb $0xdf, %al                                 # 0xdf -> port 0x60
    outb %al, $0x60                                 # 0xdf = 11011111, means set P2's A20 bit(the 1 bit) to 1
```
通过对键盘控制器的访问来打开A20端口  
第三步：
```
	lgdt gdtdesc
    movl %cr0, %eax
    orl $CR0_PE_ON, %eax
    movl %eax, %cr0
```
其中将全局段控制表载入，然后改写段寄存器CR0的PE字段，开启了保护模式。
接着ljmp $PROT_MODE_CSEG, $protcseg跳转到下一条指令的位置，进入保护模式。  
>第四步
```
.code32                                             # Assemble for 32-bit mode
protcseg:
    # Set up the protected-mode data segment registers
    movw $PROT_MODE_DSEG, %ax                       # Our data segment selector
    movw %ax, %ds                                   # -> DS: Data Segment
    movw %ax, %es                                   # -> ES: Extra Segment
    movw %ax, %fs                                   # -> FS
    movw %ax, %gs                                   # -> GS
    movw %ax, %ss                                   # -> SS: Stack Segment
    # Set up the stack pointer and call into C. The stack region is from 0--start(0x7c00)
    movl $0x0, %ebp
    movl $start, %esp
    call bootmain
```
>初始化保护模式下的段寄存器，最后跳转到bootmain进行下面的指令。


[练习4]
通过阅读bootmain.c，了解bootloader如何加载ELF文件。通过分析源代码和通过qemu来运行并调试bootloader&OS，
bootloader如何读取硬盘扇区的？
bootloader是如何加载ELF格式的OS？
>在bootmain.c中实现了两个函数，一个是readsect(void *dst, uint32_t secno)实现了读取单独一个扇区到dst的功能
```
outb(0x1F2, 1);                         // count = 1
    outb(0x1F3, secno & 0xFF);
    outb(0x1F4, (secno >> 8) & 0xFF);
    outb(0x1F5, (secno >> 16) & 0xFF);
    outb(0x1F6, ((secno >> 24) & 0xF) | 0xE0);
    outb(0x1F7, 0x20);                      // cmd 0x20 - read sectors
```
>核心代码如下，主要进行了一些参数的设置，最后一句利用0x20进行扇区读
>另一个是readseg(uintptr_t va, uint32_t count, uint32_t offset)，调用readsect，实现了读count个bytes内容的功能。  
>在主程序：
```
void
bootmain(void) {
    // read the 1st page off disk
    readseg((uintptr_t)ELFHDR, SECTSIZE * 8, 0);
    // is this a valid ELF?
    if (ELFHDR->e_magic != ELF_MAGIC) {
        goto bad;
    }
    struct proghdr *ph, *eph;
    // load each program segment (ignores ph flags)
    ph = (struct proghdr *)((uintptr_t)ELFHDR + ELFHDR->e_phoff);
    eph = ph + ELFHDR->e_phnum;
    for (; ph < eph; ph ++) {
        readseg(ph->p_va & 0xFFFFFF, ph->p_memsz, ph->p_offset);
    }
    // call the entry point from the ELF header
    // note: does not return
    ((void (*)(void))(ELFHDR->e_entry & 0xFFFFFF))();
bad:
    outw(0x8A00, 0x8A00);
    outw(0x8A00, 0x8E00);
    /* do nothing */
    while (1);
}
```
其中，首先读取了扇区的第一个page，根据这个page的内容进行判断，如果不是ELF格式的文件就报错。因为ELF文件格式的文件在第一个page中保存了一个表，其中描述了将对应ELF文件加载到的地址位置。根据这个表的情况，循环将ELF文件分别加载到内存中。然后根据入口地址，最终跳转到内核的入口位置。

[练习5]
实现函数调用堆栈跟踪函数
>代码见lab1  
>输出与给的样例一致  
>最后一行表示的函数堆栈最后的内容，即最先入栈的内容。在实模式到保护模式的转换过程中有
```start(0x7c00)
    movl $0x0, %ebp
    movl $start, %esp
    call bootmain
```
将堆栈的起始设置为0x7c00，然后设置ebp，esp。call bootmain是第一个调用的函数，同时将指令call压入栈中，所以此时的ebp的值为0x7bf8


[练习6]
请完成编码工作和回答如下问题：
1.中断描述符表（也可简称为保护模式下的中断向量表）中一个表项占多少字节？其中哪几位代表中断处理代码的入口？  
>一个中断描述符表一个表项占八个字节，其中2-3字节表示段选择子，

2.请编程完善kern/trap/trap.c中对中断向量表进行初始化的函数idt_init。在idt_init函数中，依次对所有中断入口进行初始化。使用mmu.h中的SETGATE宏，填充idt数组内容。每个中断的入口由tools/vectors.c生成，使用trap.c中声明的vectors数组即可。
3.请编程完善trap.c中的中断处理函数trap，在对时钟中断进行处理的部分填写trap函数中处理时钟中断的部分，使操作系统每遇到100次时钟中断后，调用print_ticks子程序，向屏幕上打印一行文字”100 ticks”。   
2与3实现见代码。

[Challenge 1]
扩展proj4,增加syscall功能，即增加一用户态函数（可执行一特定系统调用：获得时钟计数值），当内核初始完毕后，可从内核态返回到用户态的函数，而用户态的函数又通过系统调用得到内核态的服务
见实现代码
