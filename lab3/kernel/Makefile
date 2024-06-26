CFLAGS = -Wall -ffreestanding -nostdinc -nostdlib -nostartfiles -fno-stack-protector -g -mgeneral-regs-only
C    = aarch64-linux-gnu

LIB_PATH ?= ../lib
BSP_PATH ?= ../bsp
CFLAGS += -I$(LIB_PATH) -I$(BSP_PATH) -I..

LIB_SRCS = $(shell find $(LIB_PATH) -type f -name '*.c')
BSP_SRCS = $(shell find $(BSP_PATH) -type f -name '*.c')
LOCAL_SRCS = $(shell find . -type f -name '*.c')
SRCS = $(LOCAL_SRCS) $(LIB_SRCS) $(BSP_SRCS)
OBJS = $(patsubst ./%.c,obj/%.o,$(LOCAL_SRCS)) \
       $(patsubst $(LIB_PATH)/%.c,obj/lib/%.o,$(LIB_SRCS)) \
       $(patsubst $(BSP_PATH)/%.c,obj/bsp/%.o,$(BSP_SRCS))

QEMU_OPTS ?= 

ifeq ($(DEBUG), 1)
CFLAGS += -DDEBUG
endif

all: clean kernel8.img

obj/%.o: %.c
	mkdir -p $(dir $@)
	$(C)-gcc $(CFLAGS) -c $< -o $@

obj/start_s.o: $(BSP_PATH)/start.S
	mkdir -p $(dir $@)
	$(C)-gcc $(CFLAGS) -c $< -o $@

obj/exception_s.o: $(BSP_PATH)/exception.S
	mkdir -p $(dir $@)
	$(C)-gcc $(CFLAGS) -c $< -o $@

obj/lib/%.o: $(LIB_PATH)/%.c
	mkdir -p $(dir $@)
	$(C)-gcc $(CFLAGS) -c $< -o $@

obj/bsp/%.o: $(BSP_PATH)/%.c
	mkdir -p $(dir $@)
	$(C)-gcc $(CFLAGS) -c $< -o $@

kernel8.img: obj/start_s.o obj/exception_s.o $(OBJS)
	$(C)-ld -nostdlib $^ -T linker.ld -o kernel8.elf
	$(C)-objcopy -O binary kernel8.elf kernel8.img

clean:
	rm -rf obj kernel8.elf kernel8.img

run: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio $(QEMU_OPTS)

debug: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio -s -S $(QEMU_OPTS)
